#include <stdio.h>
#include <glm/gtc/type_ptr.hpp>

#include "sprite_renderer.h"

SpriteRenderer init_renderer(GLuint shader) {
    SpriteRenderer sr = {};
    sr.Shader = shader;

    GLfloat vertices[] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &(sr.VAO));
    glGenBuffers(1, &(sr.VBO));
    glBindVertexArray(sr.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, sr.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return sr;
}

void render(SpriteRenderer sr) {
    glUseProgram(sr.Shader);
    glBindVertexArray(sr.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void draw_sprite(SpriteRenderer sr, Texture texture, glm::vec2 position, glm::vec2 size, GLfloat rotate, glm::vec3 color) {
    glUseProgram(sr.Shader);
    glm::mat4 model;
    model = glm::translate(model, glm::vec3(position, 0.0f));

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
    model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

    model = glm::scale(model, glm::vec3(size, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(sr.Shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(sr.Shader, "spriteColor"), color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.ID);

    glBindVertexArray(sr.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void draw_game_object(SpriteRenderer sr, GameObject go, glm::vec3 color) {
    draw_sprite(sr, go.sprite, go.position, go.size, go.rotation, color); 
}

void destroy_renderer(SpriteRenderer sr) {
    glDeleteVertexArrays(1, &(sr.VAO));
    glDeleteBuffers(1, &(sr.VBO));
}
