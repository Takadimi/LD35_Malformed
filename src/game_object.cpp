#include "game_object.h"

GameObject create_game_object(Texture sprite, glm::vec2 size, glm::vec2 position, glm::vec2 velocity, float rotation) {
    GameObject go = {};
    go.sprite = sprite;
    go.size = size;
    go.position = position;
    go.velocity = velocity;
    go.rotation = rotation; 

    return go;
}
