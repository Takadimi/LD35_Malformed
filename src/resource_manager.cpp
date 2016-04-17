#include <stdio.h>
#include <assert.h>
#include <map>

#include "util.h"
#include "shader.h"
#include "resource_manager.h"

static std::map <std::string, Texture> textures;
static std::map <std::string, GLuint>  shaders;
static std::map <std::string, Map>     maps;

void init_resource_manager() {
    set_resource_path("res\\");
}

void load_texture(Texture texture, std::string name) {
    textures[name] = texture;
}

void load_map(Map map, std::string name) {
    maps[name] = map;
}

void load_shader(const char* vert_file_path, const char* frag_file_path, std::string name) {
    GLuint vertexShader     = create_shader(GL_VERTEX_SHADER, vert_file_path);
    GLuint fragmentShader   = create_shader(GL_FRAGMENT_SHADER, frag_file_path);

    shaders[name] = create_shader_program(vertexShader, fragmentShader);
}

Texture get_texture(std::string name) {
    std::map<std::string, Texture>::iterator it = textures.find(name);
    assert(it != textures.end());

    return textures.at(name);
}

Map get_map(std::string name) {
    std::map<std::string, Map>::iterator it = maps.find(name);
    assert(it != maps.end());

    return maps.at(name);
}

GLuint get_shader(std::string name) {
    std::map<std::string, GLuint>::iterator it = shaders.find(name);
    assert(it != shaders.end());

    GLuint shader = shaders.at(name);
    return shader;
}