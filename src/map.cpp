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
