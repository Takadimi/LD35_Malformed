#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "shader.h"

GLuint create_shader(GLenum shader_type, const char* shader_source_file_path) {
    char* realized_path = realize_path(shader_source_file_path);

    size_t shader_source_length = 0;
    char* shader_source = read_entire_file(realized_path, &shader_source_length);

    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }

    free(shader_source);

    return shader;
}

GLuint create_shader_program(GLuint v_shader, GLuint f_shader) {
    GLuint shader_program = glCreateProgram();

    glAttachShader(shader_program, v_shader);
    glAttachShader(shader_program, f_shader);
    glLinkProgram(shader_program);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return shader_program;
}
