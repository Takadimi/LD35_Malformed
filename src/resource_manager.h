#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <GL/gl3w.h>

#include "texture.h"
#include "map.h"

void init_resource_manager();

void load_texture(Texture texture, std::string name);
void load_shader(const char* vert_file_path, const char* frag_file_path, std::string name);
void load_map(Map map, std::string name);

Texture get_texture(std::string name);
GLuint get_shader(std::string name);
Map get_map(std::string name);

#endif