#ifndef ENEMY_H
#define ENEMY_H

#include "game_object.h"

#define ENEMY_SPAWN_INTERVAL 1.5
#define ENEMY_VELOCITY 195 
#define ENEMY_NORMAL_HEALTH 10 
#define ENEMY_OVERCHARGED_HEALTH 35 
#define ENEMY_ROTATION_OVER_TIME 0.5

enum EnemyType {
    ENEMY_NORMAL,
    ENEMY_OVERCHARGED
};

struct Enemy {
    GameObject go;
    float health;
    EnemyType type;
    bool flash_on_render;
};

int get_next_spawnable_enemy(Enemy* enemies, int enemy_count);

#endif