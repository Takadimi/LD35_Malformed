#ifndef BULLET_H
#define BULLET_H

#include "game_object.h"

#define BULLET_VELOCITY 800
#define BULLET_ROTATION_OVER_TIME 0.8
#define BULLET_MAX_LIFETIME 5.0
#define FIRE_RATE 0.3 

struct Bullet {
    GameObject go;
    float time_alive;
    bool alive;
};

#endif