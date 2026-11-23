#ifndef MOVEMENT_H
#define MOVEMENT_H

struct vec3 {
    float x;
    float y;
    float z;
};

void startWalking();
void walk(float theta);
void startBobUpDown();
void bobUpDown();
vec3 pointToAngles(vec3 point);
vec3 pointToServoAngles(vec3 point);

#endif
