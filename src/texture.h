#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/gl3w.h>

struct Texture {
    GLuint ID;
    int Width;
    int Height;
};

Texture init_texture(const char* file_path);

#endif