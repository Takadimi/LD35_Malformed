#ifndef SHADER_H
#define SHADER_H

#include <GL/gl3w.h>

GLuint create_shader(GLenum shader_type, const char* shader_source_file_path);
GLuint create_shader_program(GLuint v_shader, GLuint f_shader);

#endif SHADER_H
