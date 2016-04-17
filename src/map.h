#ifndef MAP_H
#define MAP_H

#define TILE_SIZE 64 

enum tile_types {
    GROUND,
    WALL,
    TUNNEL,
    SPAWN,

    TILE_TYPE_COUNT
};

struct Tile {
    // X and Y are positioned relative to the origin of the map that's being rendered
    int x, y, type;
};

struct Map {
    int width;
    int height;
    Tile tile_data[4096]; // Maximum area for a map
};

Map init_map(const char* file_path);

#endif