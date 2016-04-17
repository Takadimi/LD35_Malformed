#include <stdio.h>
#include <math.h>
#include <iostream>
#include <tuple>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <irrklang/irrKlang.h>

#include "game.h"
#include "game_object.h"
#include "bullet.h"
#include "enemy.h"
#include "map.h"
#include "shader.h"
#include "texture.h"
#include "util.h"
#include "resource_manager.h"

#define AB_VELOCITY 400
#define TUNNEL_ANIMATION_FREQUENCY 0.1

enum Game_State {
    GAME_RUNNING,
    GAME_OVER
};

enum Tunnel_State {
    TUNNEL_BEGIN,
    TUNNEL_MIDDLE,
    TUNNEL_END
};

enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;

Game_State curr_game_state = GAME_RUNNING;

GameObject awesome_ball;
int score = 0;
bool player_spawned = false;

#define POINT_TIMER_INTERVAL 1.0
float point_timer = 0.0f;

#define BULLET_COUNT 256
Bullet bullets[BULLET_COUNT]; // Fixed size array of bullets. Once they've all been initialized we just recycle them. As long the bullet fire speed is slow enough 256 should be fine.
int bullet_counter = 0; // Index (using modulo) into the bullets array
float fire_timer = 0.0f;

#define ENEMY_COUNT 64
Enemy enemies[ENEMY_COUNT]; // Fixed size array of enemies. If all 128 are alive, then we don't spawn them until one dies.
float enemy_spawn_timer;

Tunnel_State tunnel_state = TUNNEL_BEGIN;
float tunnel_timer = 0.0f;
glm::vec2 camera_pos;

irrklang::ISoundEngine *SoundEngine = irrklang::createIrrKlangDevice();

#define PI 3.14159265
float rad2deg(float radians) {
    return radians * (180/PI);
}

float deg2rad(float degrees) {
    return degrees * (PI/180);
}

Direction VectorDirection(glm::vec2 target) {
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, -1.0f),
        glm::vec2(-1.0f, 0.0f)
    };

    GLfloat max = 0.0f;
    GLuint best_match = -1;

    for (GLuint i = 0; i < 4; i++) {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max) {
            max = dot_product;
            best_match = i;
        }
    }

    return (Direction) best_match;
}

static Collision check_tile_collision(GameObject player, Tile tile) {
    GLfloat player_radius = player.size.x / 2; // Treating size.x / 2 as radius of bounding circle. Yes, this is janky as fuck, no I won't fix it :)

    glm::vec2 center(player.position + player_radius);
    glm::vec2 aabb_half_extents(TILE_SIZE / 2, TILE_SIZE / 2);
    glm::vec2 aabb_center(
        tile.x + aabb_half_extents.x,
        tile.y + aabb_half_extents.y
    );

    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;

    if (glm::length(difference) <= player_radius) {
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    } else {
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
    }
}

static Collision check_game_object_collision(GameObject player, GameObject go2) {
    GLfloat player_radius = player.size.x / 2; // Treating size.x / 2 as radius of bounding circle. Yes, this is janky as fuck, no I won't fix it :)

    glm::vec2 center(player.position + player_radius);
    glm::vec2 aabb_half_extents(go2.size.x / 2, go2.size.y / 2);
    glm::vec2 aabb_center(
        go2.position.x + aabb_half_extents.x,
        go2.position.y + aabb_half_extents.y
    );

    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;

    if (glm::length(difference) <= player_radius) {
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    } else {
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
    }
}

static Collision check_bullet_collision(GameObject enemy, Bullet bullet) {
    if (!bullet.alive) return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));

    GLfloat enemy_radius = enemy.size.x / 2; // Treating size.x / 2 as radius of bounding circle. Yes, this is janky as fuck, no I won't fix it :)

    glm::vec2 center(enemy.position + enemy_radius);
    glm::vec2 aabb_half_extents(bullet.go.size.x / 2, bullet.go.size.y / 2);
    glm::vec2 aabb_center(
        bullet.go.position.x + aabb_half_extents.x,
        bullet.go.position.y + aabb_half_extents.y
    );

    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;

    if (glm::length(difference) <= enemy_radius) {
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    } else {
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
    }
}

GameState game_init(float screen_width, float screen_height) {
    GameState gs = {};
    gs.screen_width = screen_width;
    gs.screen_height = screen_height;

    init_resource_manager();

    load_shader("shaders\\sprite.vert", "shaders\\sprite.frag", "sprite");
    load_shader("shaders\\font.vert", "shaders\\font.frag", "font");

    GLuint shader_program = get_shader("sprite");
    glUseProgram(shader_program);
    glUniform1i(glGetUniformLocation(shader_program, "image"), 0);
    gs.projection = glm::ortho(0.0f, gs.screen_width, gs.screen_height, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(gs.projection));
    gs.camera = glm::mat4();
    gs.camera = glm::translate(gs.camera, glm::vec3(camera_pos, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, glm::value_ptr(gs.camera));

    load_texture(init_texture("textures\\awesomeface.png"), "awesomeface");
    load_texture(init_texture("textures\\sprite_one.png"), "ground");
    load_texture(init_texture("textures\\sprite_two.png"), "wall");
    load_texture(init_texture("textures\\sprite_three.png"), "tunnel");
    load_texture(init_texture("textures\\sprite_four.png"), "player_shield");
    load_texture(init_texture("textures\\sprite_five.png"), "player_normal");
    load_texture(init_texture("textures\\sprite_six.png"), "bullet");
    load_texture(init_texture("textures\\sprite_seven.png"), "tunnel_two");
    load_texture(init_texture("textures\\sprite_eight.png"), "tunnel_three");
    load_texture(init_texture("textures\\sprite_nine.png"), "enemy");
    load_texture(init_texture("textures\\sprite_ten.png"), "reticule");

    load_texture(init_texture("fonts\\ExportedFont.tga"), "font_one");

    load_map(init_map("maps\\level1.tmap"), "level_one");

    SpriteRenderer renderer = init_renderer(shader_program);
    gs.sr = renderer;

    shader_program = get_shader("font");
    glUseProgram(shader_program);
    FontRenderer font_renderer = init_font_renderer(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(gs.projection));
    gs.fr = font_renderer;

    awesome_ball = create_game_object(get_texture("player_normal"), glm::vec2(64.0f, 64.0f), glm::vec2(400.0f, 300.0f), glm::vec2(0.0f, 0.0f), 0.0f);
    awesome_ball.position.x -= (awesome_ball.size.x / 2);
    awesome_ball.position.y -= (awesome_ball.size.y / 2);

    return gs;
}

void game_update_and_render(GameState* gs, GLfloat dt) {
    Map level_map = get_map("level_one");
    glm::vec2 mouse_pos_in_world = gs->mouse_pos - camera_pos;

    if (curr_game_state == GAME_RUNNING) {
        awesome_ball.velocity = glm::vec2(0, 0); // Zero out velocity from previous frame

        if (gs->keys[GLFW_KEY_W]) {
            awesome_ball.velocity.y = -AB_VELOCITY * dt;
        }

        if (gs->keys[GLFW_KEY_S]) {
            awesome_ball.velocity.y = AB_VELOCITY * dt;
        }

        if (gs->keys[GLFW_KEY_A]) {
            awesome_ball.velocity.x = -AB_VELOCITY * dt;
        }

        if (gs->keys[GLFW_KEY_D]) {
            awesome_ball.velocity.x = AB_VELOCITY * dt;
        }

        awesome_ball.position += awesome_ball.velocity; 

        glm::vec2 mouse_pos_in_world = gs->mouse_pos - camera_pos;
        awesome_ball.rotation = atan2(mouse_pos_in_world.y - (awesome_ball.position.y + (awesome_ball.size.y / 2)), mouse_pos_in_world.x - (awesome_ball.position.x + (awesome_ball.size.x / 2)));
        awesome_ball.rotation += PI / 2;

        fire_timer += dt;
        if (gs->left_mouse_held) {
            if (fire_timer - FIRE_RATE >= 0.0f || bullet_counter == 0) {
                fire_timer = 0.0f;
                bullet_counter++;
                int bullet_index = bullet_counter%(sizeof(bullets)/sizeof(bullets[0]));
                if (!bullets[bullet_index].alive) {
                    bullets[bullet_index].alive = true;
                    bullets[bullet_index].go = create_game_object(get_texture("bullet"), glm::vec2(32.0f, 32.0f), glm::vec2(awesome_ball.position.x + (awesome_ball.size.x / 2), awesome_ball.position.y + (awesome_ball.size.y / 2)), glm::vec2((BULLET_VELOCITY * sin(awesome_ball.rotation)) * dt, (-BULLET_VELOCITY * cos(awesome_ball.rotation)) * dt), awesome_ball.rotation);
                    bullets[bullet_index].go.position.x -= bullets[bullet_index].go.size.x / 2;
                    bullets[bullet_index].go.position.y -= bullets[bullet_index].go.size.y / 2;
                } else {
                    bullets[bullet_index].go.velocity = glm::vec2((BULLET_VELOCITY * sin(awesome_ball.rotation)) * dt, (-BULLET_VELOCITY * cos(awesome_ball.rotation)) * dt);
                    bullets[bullet_index].go.rotation = awesome_ball.rotation;
                    bullets[bullet_index].go.position = glm::vec2(awesome_ball.position.x + (awesome_ball.size.x / 2), awesome_ball.position.y + (awesome_ball.size.y / 2));
                    bullets[bullet_index].go.position.x -= bullets[bullet_index].go.size.x / 2;
                    bullets[bullet_index].go.position.y -= bullets[bullet_index].go.size.y / 2;
                }

                SoundEngine->play2D("..\\res\\audio\\shoot.wav", GL_FALSE);
                // SoundEngine->play2D("C:\\Users\\Ethan\\Sandbox\\c++\\LD35\\res\\audio\\shoot.wav", GL_FALSE);
            }
        } else if (gs->right_mouse_held) {
        }

        GLfloat player_radius = awesome_ball.size.x  / 2;
        for (int i = 0; i < level_map.width * level_map.height; i++) {
            Tile tile = level_map.tile_data[i];

            if (tile.type == 1) {
                Collision collision = check_tile_collision(awesome_ball, tile);

                if (std::get<0>(collision)) {
                    Direction dir = std::get<1>(collision);
                    glm::vec2 diff_vector = std::get<2>(collision);

                    if (dir == LEFT || dir == RIGHT) {
                        awesome_ball.velocity.x = -awesome_ball.velocity.x;

                        GLfloat penetration = player_radius - std::abs(diff_vector.x);
                        if (dir == LEFT) {
                            awesome_ball.position.x += penetration;
                        } else {
                            awesome_ball.position.x -= penetration;
                        }
                    } else {
                        awesome_ball.velocity.y = -awesome_ball.velocity.y;

                        GLfloat penetration = player_radius - std::abs(diff_vector.y);
                        if (dir == UP) {
                            awesome_ball.position.y -= penetration;
                        } else {
                            awesome_ball.position.y += penetration;
                        }
                    }
                }

                for (int j = 0; j < ENEMY_COUNT; j++) {
                    if (enemies[j].health > 0.0f) {
                        Collision enemy_collision = check_game_object_collision(awesome_ball, enemies[j].go);

                        if (std::get<0>(enemy_collision)) {
                            curr_game_state = GAME_OVER;
                            break;
                        }
                    }
                }

                for (int j = 0; j < BULLET_COUNT; j++) {
                    if (bullets[j].alive) {
                        Collision bullet_wall_collision = check_tile_collision(bullets[j].go, tile);

                        if (std::get<0>(bullet_wall_collision)) {
                            bullets[j].time_alive = 0.0f;
                            bullets[j].alive = false;
                        }
                    }
                }
            }
        }

        enemy_spawn_timer += dt;
        if (enemy_spawn_timer - ENEMY_SPAWN_INTERVAL >= 0.0f) {
            enemy_spawn_timer = 0.0f;
            int next_spawnable_enemy = get_next_spawnable_enemy(enemies, ENEMY_COUNT);

            if (next_spawnable_enemy != -1) {
                enemies[next_spawnable_enemy].go = create_game_object(get_texture("enemy"), glm::vec2(64.0f, 64.0f), get_random_spawn_location(get_map("level_one")), glm::vec2(0,0), 0.0f);
                enemies[next_spawnable_enemy].flash_on_render = false;

                if ((rand() % 10) < 2) {
                    enemies[next_spawnable_enemy].type = ENEMY_OVERCHARGED;
                    enemies[next_spawnable_enemy].health = (float) ENEMY_OVERCHARGED_HEALTH;
                } else {
                    enemies[next_spawnable_enemy].type = ENEMY_NORMAL;
                    enemies[next_spawnable_enemy].health = (float) ENEMY_NORMAL_HEALTH;
                }
            }
        }

        for (int i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].health > 0.0f) {
                // Determine enemy velocity (direction of player)
                enemies[i].go.velocity = glm::normalize(awesome_ball.position - enemies[i].go.position);
                enemies[i].go.velocity *= (dt * (float)ENEMY_VELOCITY);

                enemies[i].go.position += enemies[i].go.velocity; 

                GLfloat enemy_radius = enemies[i].go.size.x / 2;
                for (int j = 0; j < level_map.width * level_map.height; j++) {
                    Tile tile = level_map.tile_data[j];

                    if (tile.type == 1) {
                        Collision collision = check_tile_collision(enemies[i].go, tile);

                        if (std::get<0>(collision)) {
                            Direction dir = std::get<1>(collision);
                            glm::vec2 diff_vector = std::get<2>(collision);

                            if (dir == LEFT || dir == RIGHT) {
                                enemies[i].go.velocity.x = -enemies[i].go.velocity.x;

                                GLfloat penetration = enemy_radius - std::abs(diff_vector.x);
                                if (dir == LEFT) {
                                    enemies[i].go.position.x += penetration;
                                } else {
                                    enemies[i].go.position.x -= penetration;
                                }
                            } else {
                                enemies[i].go.velocity.y = -enemies[i].go.velocity.y;

                                GLfloat penetration = enemy_radius - std::abs(diff_vector.y);
                                if (dir == UP) {
                                    enemies[i].go.position.y -= penetration;
                                } else {
                                    enemies[i].go.position.y += penetration;
                                }
                            }
                        }
                    }
                }

                for (int j = 0; j < ENEMY_COUNT; j++) {
                    if (j == i) continue;
                    if (enemies[j].health <= 0.0f) continue;

                    Collision collision = check_game_object_collision(enemies[i].go, enemies[j].go);

                    if (std::get<0>(collision)) {
                        Direction dir = std::get<1>(collision);
                        glm::vec2 diff_vector = std::get<2>(collision);

                        if (dir == LEFT || dir == RIGHT) {
                            enemies[i].go.velocity.x = -enemies[i].go.velocity.x;

                            GLfloat penetration = enemy_radius - std::abs(diff_vector.x);
                            if (dir == LEFT) {
                                enemies[i].go.position.x += penetration;
                            } else {
                                enemies[i].go.position.x -= penetration;
                            }
                        } else {
                            enemies[i].go.velocity.y = -enemies[i].go.velocity.y;

                            GLfloat penetration = enemy_radius - std::abs(diff_vector.y);
                            if (dir == UP) {
                                enemies[i].go.position.y -= penetration;
                            } else {
                                enemies[i].go.position.y += penetration;
                            }
                        }
                    }
                }

                for (int j = 0; j < BULLET_COUNT; j++) {
                    Collision collision = check_bullet_collision(enemies[i].go, bullets[j]);

                    if (std::get<0>(collision)) {
                        SoundEngine->play2D("..\\res\\audio\\kill_enemy.wav", GL_FALSE);
                        // SoundEngine->play2D("C:\\Users\\Ethan\\Sandbox\\c++\\LD35\\res\\audio\\kill_enemy.wav", GL_FALSE);
                        bullets[j].time_alive = 0.0f;
                        bullets[j].alive = false;
                        enemies[i].health -= 5.0f;
                        enemies[i].flash_on_render = true;
                        if (enemies[i].health <= 0.0f) {
                            score += 100;
                        }
                    }
                }
            }
        }

        point_timer += dt;
        if (point_timer - POINT_TIMER_INTERVAL >= 0.0f) {
            score += 10;
            point_timer = 0.0f;
        }
    } else if (curr_game_state == GAME_OVER) {
        if (gs->keys[GLFW_KEY_R]) {
            for (int i = 0; i < BULLET_COUNT; i++) {
                bullets[i] = {};
            }

            bullet_counter = 0;

            for (int i = 0; i < ENEMY_COUNT; i++) {
                enemies[i] = {};
            }

            score = 0; 
            player_spawned = false;
            curr_game_state = GAME_RUNNING;
        }
    }

    /****************************** RENDER ******************************************************************************/

    /* LOCK CAMERA TO CENTER OF PLAYER */
    gs->camera = glm::mat4();
    camera_pos.x = -(awesome_ball.position.x + (awesome_ball.size.x / 2) - (gs->screen_width / 2));
    camera_pos.y = -(awesome_ball.position.y + (awesome_ball.size.y / 2) - (gs->screen_height / 2));
    gs->camera = glm::translate(gs->camera, glm::vec3(camera_pos, 0.0f));

    glUseProgram(gs->sr.Shader);
    glUniformMatrix4fv(glGetUniformLocation(get_shader("sprite"), "view"), 1, GL_FALSE, glm::value_ptr(gs->camera));

    // ------------------- MAP ----------------------------------------------------------
    bool switch_tunnel_state = (tunnel_timer - TUNNEL_ANIMATION_FREQUENCY >= 0.0f);

    for (int i = 0; i < level_map.height; i++) {
        for (int j = 0; j < level_map.width; j++) {
            Tile tile = level_map.tile_data[(i*level_map.width) + j];
            Texture tile_texture;

            switch (tile.type) {
                case 0:
                    tile_texture = get_texture("ground");
                    break;
                case 1:
                    tile_texture = get_texture("wall");
                    break;
                case 2:
                    if (switch_tunnel_state) {
                        tunnel_timer = 0.0f;

                        if (tunnel_state == TUNNEL_BEGIN) {
                            tunnel_state = TUNNEL_MIDDLE;
                        } else if (tunnel_state == TUNNEL_MIDDLE) {
                            tunnel_state = TUNNEL_END;
                        } else if (tunnel_state == TUNNEL_END) {
                            tunnel_state = TUNNEL_BEGIN;
                        }
                    }

                    if (tunnel_state == TUNNEL_BEGIN) {
                        tile_texture = get_texture("tunnel");
                    } else if (tunnel_state == TUNNEL_MIDDLE) {
                        tile_texture = get_texture("tunnel_two");
                    } else if (tunnel_state == TUNNEL_END) {
                        tile_texture = get_texture("tunnel_three");
                    }

                    break;
                case 3:
                    tile_texture = get_texture("ground");                    
                    if (!player_spawned) {
                        awesome_ball.position = glm::vec2(tile.x, tile.y);
                        player_spawned = true;
                    }
                    break;
                default:
                    tile_texture = get_texture("awesomeface");
                    break;
            }

            draw_sprite(gs->sr, tile_texture, glm::vec2(tile.x, tile.y), glm::vec2(TILE_SIZE, TILE_SIZE), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    tunnel_timer += dt;

    // ------------------- ENEMIES ------------------------------------------------
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].health > 0.0f) {
            glm::vec3 enemy_color;

            if (enemies[i].type == ENEMY_OVERCHARGED) {
                enemy_color = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                enemy_color = glm::vec3(0.059f, 0.659f, 0.078f);
            }

            if (enemies[i].flash_on_render) {
                enemy_color = glm::vec3(1.0f, 1.0f, 1.0f);
                enemies[i].flash_on_render = false;
            }

            enemies[i].go.rotation += ENEMY_ROTATION_OVER_TIME;
            draw_game_object(gs->sr, enemies[i].go, enemy_color);
        }
    }

    // ------------------- BULLETS ------------------------------------------------
    for (int i = 0; i < sizeof(bullets) / sizeof(bullets[0]); i++) {
        if (bullets[i].alive) {
            if (bullets[i].time_alive >= BULLET_MAX_LIFETIME) {
                bullets[i].time_alive = 0.0f;
                bullets[i].alive = false;
            }

            bullets[i].go.position += bullets[i].go.velocity;
            bullets[i].go.rotation += BULLET_ROTATION_OVER_TIME;
            draw_game_object(gs->sr, bullets[i].go, glm::vec3(1.0f, 1.0f, 1.0f));
            bullets[i].time_alive += dt;
        }
    }

    // -------------------- PLAYER ------------------------------------------------
    draw_game_object(gs->sr, awesome_ball, glm::vec3(1.0f, 1.0f, 1.0f));

    // -------------------- CURSOR RETICULE ---------------------------------------
    draw_sprite(gs->sr, get_texture("reticule"), glm::vec2(mouse_pos_in_world.x - 16.0f, mouse_pos_in_world.y - 16.0f), glm::vec2(32.0f, 32.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    // -------------------- TEXT -------------------------------------------------- 
    if (curr_game_state == GAME_OVER) {
        char* game_over_message = "GAME OVER!";
        float string_x = 170.0f;
        for (int i = 0; i < strlen(game_over_message); i++) {
            draw_bitmap_char(gs->fr, game_over_message[i], get_texture("font_one"), glm::vec2(string_x, 200.0f), glm::vec2(1024.0f, 1024.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
            string_x += 48.0f;
        }

        char score_message[255];
        int message_char_count = sprintf(score_message, "SCORE: %d", score);
        string_x = 220.0f;
        for (int i = 0; i < message_char_count; i++) {
            draw_bitmap_char(gs->fr, score_message[i], get_texture("font_one"), glm::vec2(string_x, 300.0f), glm::vec2(512.0f, 512.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
            string_x += 32.0f;
        }

        char* restart_message = "PRESS 'R' TO RESTART";
        string_x = 100.0f;
        for (int i = 0; i < strlen(restart_message); i++) {
            draw_bitmap_char(gs->fr, restart_message[i], get_texture("font_one"), glm::vec2(string_x, 400.0f), glm::vec2(512.0f, 512.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
            string_x += 32.0f;
        }
    }

    {
        char test_str[255];
        int string_char_count = sprintf(test_str, "SCORE: %d", score);
        float starting_string_x = 10.0f;
        float string_x = starting_string_x;
        float string_y = 10.0f;
        for (int i = 0; i < string_char_count; i++) {
            draw_bitmap_char(gs->fr, test_str[i], get_texture("font_one"), glm::vec2(string_x, string_y), glm::vec2(256.0f, 256.0f), 0.0f, glm::vec3((102.0f/255.0f), (255.0f/255.0f), (153.0f/255.0f)));
            string_x += 16.0f;
        }
    }

    {
        int alive_enemies = 0;
        for (int i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].health > 0.0f) alive_enemies++;
        }

        char test_str[255];
        int string_char_count = sprintf(test_str, "ENEMIES ON SCREEN: %d", alive_enemies);
        float starting_string_x = 10.0f;
        float string_x = starting_string_x;
        float string_y = 40.0f;
        for (int i = 0; i < string_char_count; i++) {
            draw_bitmap_char(gs->fr, test_str[i], get_texture("font_one"), glm::vec2(string_x, string_y), glm::vec2(256.0f, 256.0f), 0.0f, glm::vec3((102.0f/255.0f), (255.0f/255.0f), (153.0f/255.0f)));
            string_x += 16.0f;
        }
    }

    {
        char test_str[] = "MALFORMED - CLICK TO SHOOT";
        float starting_string_x = 10.0f;
        float string_x = starting_string_x;
        float string_y = 580.0f;
        for (int i = 0; i < sizeof(test_str); i++) {
            draw_bitmap_char(gs->fr, test_str[i], get_texture("font_one"), glm::vec2(string_x, string_y), glm::vec2(256.0f, 256.0f), 0.0f, glm::vec3((102.0f/255.0f), (255.0f/255.0f), (153.0f/255.0f)));
            string_x += 16.0f;
        }
    }
}

void game_destroy(GameState* gs) {
    destroy_renderer(gs->sr);
}