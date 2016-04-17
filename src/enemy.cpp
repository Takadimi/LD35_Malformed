#include <stdio.h>

#include "enemy.h"

int get_next_spawnable_enemy(Enemy* enemies, int enemy_count) {
    for (int i = 0; i < enemy_count; i++) {
        if (enemies[i].health <= 0.0f) {
            return i;
        }
    }

    return -1;
}