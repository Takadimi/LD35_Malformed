#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "texture.h"

#define SPRITE_SHEET_SIZE 256
#define SPRITE_SIZE       32
#define SPRITES_PER_ROW   8

struct FontRenderer {
    GLuint VAO;
    GLuint VBO;
    GLuint Shader;
};

FontRenderer init_font_renderer(GLuint shader_program);
void draw_bitmap_char(FontRenderer fr, char bitmap_char, Texture texture, glm::vec2 position, glm::vec2 size, GLfloat rotate, glm::vec3 color);
void destroy_renderer(FontRenderer fr);

#endif