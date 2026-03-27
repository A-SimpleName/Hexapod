#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "servocontrols.h"


struct vec3 {
    float x;
    float y;
    float z;
};

struct vec2 {
    float x;
    float y;
};
void moveLine(int servoId, vec3 start, vec3 end);
vec3 pointToAngles(vec3 point);



#endif