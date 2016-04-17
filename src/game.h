#ifndef GAME_H
#define GAME_H

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "sprite_renderer.h"
#include "font_renderer.h"

struct GameState {
    float screen_width;
    float screen_height;
    SpriteRenderer sr;
    FontRenderer fr;
    glm::mat4 projection;
    glm::mat4 camera;
    int keys[256];
    int keys_held[256];
    glm::vec2 mouse_pos;
    bool left_mouse_held;
    bool right_mouse_held;
};

GameState game_init(float screen_width, float screen_height);
void game_update_and_render(GameState* gs, GLfloat delta);
void game_destroy(GameState* gs);

#endif