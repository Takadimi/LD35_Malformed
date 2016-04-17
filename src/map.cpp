#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "map.h"

Map init_map(const char* file_path) {
    Map map = {};

    char* realized_path = realize_path(file_path);

    size_t file_size = 0;
    char* map_data = read_entire_file(realized_path, &file_size);

    int tile_data_index = 0;
    for (int i = 0; i <= file_size; i++) {
        if (map_data[i] == '\0') {
            map.height += 1;
            break;
        }

        if (map_data[i] == '\r') {
            continue;
        }

        if (map_data[i] == '\n') {
            map.width = 0;
            map.height += 1;

            continue;
        }

        map.tile_data[tile_data_index].type = map_data[i] - '0';
        map.tile_data[tile_data_index].x    = map.width  * TILE_SIZE;
        map.tile_data[tile_data_index].y    = map.height * TILE_SIZE;
        map.width += 1;
        tile_data_index++;
    }

    free(map_data);
    return map;
}

glm::vec2 get_random_spawn_location(Map map) {
    int enemy_spawn = rand() % 8;
    int number_of_enemy_spawns_seen = 0;
    glm::vec2 last_unchosen_location;

    for (int i = 0; i < map.width*map.height; i++) {
        if (map.tile_data[i].type == 2) {
            if (enemy_spawn == number_of_enemy_spawns_seen) {
                return glm::vec2(map.tile_data[i].x, map.tile_data[i].y);
            } else {
                number_of_enemy_spawns_seen++;
                last_unchosen_location = glm::vec2(map.tile_data[i].x, map.tile_data[i].y);
           }
        }
    } 

    return last_unchosen_location; // In case for some strange reason that random number generator doesn't work as intended, get something valid to return. I'm a very paranoid person :)
}
