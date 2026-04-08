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

void walk(float theta);
void calcCurrentPoint(vec3 &startingPoint, vec3 &endPoint, vec3 &current_point_line, vec3 &current_point_bezier, float t);
void calcCurrentAngles(vec3 &current_point_line, vec3 &current_point_bezier, vec3 &angles_line, vec3 &angles_bezier, float t);
void calcPoints(vec3 p1 [], vec3 p2 [], float theta);
float toRad(float deg);
float lerp(float p1, float p2, float t);
vec3 pointToAngles(vec3 point);



#endif