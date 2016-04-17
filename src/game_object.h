#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>

#include "texture.h"

struct GameObject {
    Texture sprite;
    glm::vec2 size;
    glm::vec2 position;
    glm::vec2 velocity;
    float rotation;
};

GameObject create_game_object(Texture sprite, glm::vec2 size, glm::vec2 position, glm::vec2 velocity, float rotation);

#endif