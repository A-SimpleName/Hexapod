#include "algorithm.h"
#include "calibrations.h"
#include "servocontrols.h"
#include "movement.h"

#define HEIGHT  100.0f


void moveLine(int servoId, vec3 start, vec3 end) {

}

vec3 pointToAngles(vec3 point) {

    vec3 angles;
    vec2 point2d;
    float d;

    angles.x = atan2f(point.z, point.x);

    point2d.x = sqrtf(point.x * point.x + point.z * point.z) - L1;
    point2d.y = HEIGHT + point.y;

    d = sqrtf(point2d.x * point2d.x + point2d.y * point2d.y);

    float cosAngle = (L2*L2 + L3*L3 - d*d)/(2*L2*L3);
    if (cosAngle > 1.0f) cosAngle = 1.0f;
    if (cosAngle < -1.0f) cosAngle = -1.0f;
    angles.z = acosf(cosAngle);

    angles.y = (PI/2) - atan2f(point2d.y, point2d.x) + asinf(sinf(angles.z) * L3 / d);

    angles.x = angles.x * 180.0f / PI;
    angles.y = angles.y * 180.0f / PI;
    angles.z = angles.z * 180.0f / PI;

    return angles;
}