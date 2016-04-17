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
#include "map.h"
#include "shader.h"
#include "texture.h"
#include "util.h"
#include "resource_manager.h"

#define AB_VELOCITY 400
#define TUNNEL_ANIMATION_FREQUENCY 0.1

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

GameObject awesome_ball;
bool player_spawned = false;
Bullet bullets[256]; // Fixed size array of bullets. Once they've all been initialized we just recycle them. As long the bullet fire speed is slow enough 256 should be fine.
int bullet_counter = 0; // Index (using modulo) into the bullets array
float fire_timer = 0.0f;

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

static Collision check_collision(GameObject player, Tile tile) {
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

            SoundEngine->play2D("C:\\Users\\Ethan\\Sandbox\\c++\\LD35\\res\\audio\\shoot.wav", GL_FALSE);
        }
    } else if (gs->right_mouse_held) {
        // bullets[0].alive = false;
    }

    Map level_map = get_map("level_one");
    GLfloat player_radius = awesome_ball.size.x  / 2;
    GLboolean x_collision_detected = GL_FALSE;
    GLboolean y_collision_detected = GL_FALSE;
    for (int i = 0; i < level_map.width * level_map.height; i++) {
        Tile tile = level_map.tile_data[i];

        if (tile.type == 1 || tile.type == 2) {
            Collision collision = check_collision(awesome_ball, tile);

            if (std::get<0>(collision)) {
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);

                if (dir == LEFT || dir == RIGHT) {
                    x_collision_detected = GL_TRUE;
                    awesome_ball.velocity.x = -awesome_ball.velocity.x;

                    GLfloat penetration = player_radius - std::abs(diff_vector.x);
                    if (dir == LEFT) {
                        awesome_ball.position.x += penetration;
                    } else {
                        awesome_ball.position.x -= penetration;
                    }
                } else {
                    y_collision_detected = GL_TRUE;
                    awesome_ball.velocity.y = -awesome_ball.velocity.y;

                    GLfloat penetration = player_radius - std::abs(diff_vector.y);
                    if (dir == UP) {
                        awesome_ball.position.y -= penetration;
                    } else {
                        awesome_ball.position.y += penetration;
                    }
                }
            }
        }
    }

    gs->camera = glm::mat4();
    // if (!x_collision_detected) camera_pos.x -= awesome_ball.velocity.x;
    // if (!y_collision_detected) camera_pos.y -= awesome_ball.velocity.y;
    // gs->camera = glm::translate(gs->camera, glm::vec3(camera_pos, 0.0f));
    /* LOCK CAMERA TO CENTER OF PLAYER */
    camera_pos.x = -(awesome_ball.position.x + (awesome_ball.size.x / 2) - (gs->screen_width / 2));
    camera_pos.y = -(awesome_ball.position.y + (awesome_ball.size.y / 2) - (gs->screen_height / 2));
    gs->camera = glm::translate(gs->camera, glm::vec3(camera_pos, 0.0f));

    glUseProgram(gs->sr.Shader);
    glUniformMatrix4fv(glGetUniformLocation(get_shader("sprite"), "view"), 1, GL_FALSE, glm::value_ptr(gs->camera));

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

    draw_game_object(gs->sr, awesome_ball, glm::vec3(1.0f, 1.0f, 1.0f));
}

void game_destroy(GameState* gs) {
    destroy_renderer(gs->sr);
}