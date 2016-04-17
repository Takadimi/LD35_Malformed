#include <stdio.h>
#include <stb_image.h>

#include "util.h"
#include "texture.h"

Texture init_texture(const char* file_path) {
    Texture texture = {};

    char* realized_file_path = realize_path(file_path);
    char terminated_file_path[512];
    strcpy(terminated_file_path, realized_file_path);

    int n;
    unsigned char* data = stbi_load(terminated_file_path, &texture.Width, &texture.Height, &n, 0);

    if (!data) {
        printf("Failed: %s\n", stbi_failure_reason());
        return texture;
    }

    glGenTextures(1, &texture.ID);
    glBindTexture(GL_TEXTURE_2D, texture.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.Width, texture.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(texture.ID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return texture;
}
