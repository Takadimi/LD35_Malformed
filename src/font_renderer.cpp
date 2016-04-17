#include <stdio.h>
#include <glm/gtc/type_ptr.hpp>

#include "font_renderer.h"

FontRenderer init_font_renderer(GLuint shader) {
    FontRenderer fr = {};
    fr.Shader = shader; 

    float bitmap_uv_max = ((float)SPRITE_SIZE / SPRITE_SHEET_SIZE); // Based on the sprite sheet size and sprite size, determine the max.

    // UV Coords start with first bitmap character selected. Use uniform to offset to different chars.
    GLfloat vertices[] = {
        0.0f,          bitmap_uv_max, 0.0f,          bitmap_uv_max,
        bitmap_uv_max, 0.0f,          bitmap_uv_max, 0.0f,
        0.0f,          0.0f,          0.0f,          0.0f,

        0.0f,          bitmap_uv_max, 0.0f,          bitmap_uv_max,
        bitmap_uv_max, bitmap_uv_max, bitmap_uv_max, bitmap_uv_max,
        bitmap_uv_max, 0.0f,          bitmap_uv_max, 0.0f
    };

    glGenVertexArrays(1, &(fr.VAO));
    glGenBuffers(1, &(fr.VBO));
    glBindVertexArray(fr.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, fr.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return fr;
}

/*
    A = 65 Ascii
    65 - 32 = 33
    33 / SPRITES_PER_ROW(8) = 4
    33 % SPRITES_PER_ROW(8) = 1
    tex_coords_offset = vec2(1/SPRITES_PER_ROW(8), 4/SPRITES_PER_ROW(8)) = vec2(.125, .5)
*/

void draw_bitmap_char(FontRenderer fr, char bitmap_char, Texture texture, glm::vec2 position, glm::vec2 size, GLfloat rotate, glm::vec3 color) {
    glUseProgram(fr.Shader);
    glm::mat4 model;
    model = glm::translate(model, glm::vec3(position, 0.0f));

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
    model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

    model = glm::scale(model, glm::vec3(size, 1.0f));

    // Determine offset
    int ascii_index = bitmap_char - ' '; // Chop off first 31 chars in ascii table
    // glm::vec2 tex_coords_offset = glm::vec2(((float)ascii_index % 8.0f) / 8.0f, ((float)ascii_index / 8.0f) / 8.0f);
    glm::vec2 tex_coords_offset;
    tex_coords_offset.x = (float) (ascii_index % 8) / 8;
    tex_coords_offset.y = (float) (ascii_index / 8) / 8;
    // printf("Tex Coords Offset: %fx%f\n", tex_coords_offset.x, tex_coords_offset.y);
    glUniform2f(glGetUniformLocation(fr.Shader, "texCoordsOffset"), tex_coords_offset.x, tex_coords_offset.y);

    glUniformMatrix4fv(glGetUniformLocation(fr.Shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(fr.Shader, "spriteColor"), color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.ID);

    glBindVertexArray(fr.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
