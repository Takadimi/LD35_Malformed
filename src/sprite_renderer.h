#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "texture.h"
#include "game_object.h"

struct SpriteRenderer {
    GLuint VAO;
    GLuint VBO;
    GLuint Shader;
};

SpriteRenderer init_renderer(GLuint shader_program);
void render(SpriteRenderer sr);
void draw_sprite(SpriteRenderer sr, Texture texture, glm::vec2 position, glm::vec2 size, GLfloat rotate, glm::vec3 color);
void draw_game_object(SpriteRenderer sr, GameObject go, glm::vec3 color);
void destroy_renderer(SpriteRenderer sr);

#endif