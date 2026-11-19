#include "algorithm.h"
#include "calibrations.h"
#include "servocontrols.h"
#include "movement.h"

#define HEIGHT  100.0f

static float t = 0.0f;
static float v = 0.2f;
static int cycleCtr = 0;
    
static vec3 startingPoints [NUM_LEGS];
static vec3 endPoints [NUM_LEGS];

static vec3 current_point_line [NUM_LEGS];
static vec3 current_point_bezier [NUM_LEGS];

static vec3 angles_line [NUM_LEGS];
static vec3 angles_bezier [NUM_LEGS];

namespace {
    constexpr float BOB_STANCE_MM = 35.0f;
    constexpr float BOB_AMPLITUDE_MM = 10.0f;
    constexpr unsigned long BOB_PERIOD_MS = 2400;
    constexpr unsigned long BOB_SETTLE_MS = 1000;
    constexpr unsigned long BOB_FRAME_MS = 20;

    unsigned long bobStartedAtMs = 0;
    unsigned long bobLastFrameMs = 0;
    vec3 bobStartAngles[NUM_LEGS];
}

void startBobUpDown() {
    bobStartedAtMs = millis();
    bobLastFrameMs = bobStartedAtMs - BOB_FRAME_MS;
    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        bobStartAngles[leg] = {
            static_cast<float>(getLastServoAngle(leg, 0)),
            static_cast<float>(getLastServoAngle(leg, 1)),
            static_cast<float>(getLastServoAngle(leg, 2))
        };
    }
}

void bobUpDown() {
    const unsigned long now = millis();
    if (now - bobLastFrameMs < BOB_FRAME_MS) {
        delay(1);
        return;
    }
    bobLastFrameMs = now;

    const unsigned long elapsed = now - bobStartedAtMs;
    vec3 foot = {BOB_STANCE_MM, 0.0f, BOB_STANCE_MM};
    if (elapsed >= BOB_SETTLE_MS) {
        const float phase = 2.0f * PI *
            ((elapsed - BOB_SETTLE_MS) % BOB_PERIOD_MS) / BOB_PERIOD_MS;
        // Keep each foot's horizontal position fixed; body height varies from 80 to 100 mm.
        foot.y = BOB_AMPLITUDE_MM * (cosf(phase) - 1.0f);
    }
    const vec3 target = pointToServoAngles(foot);

    const float progress = elapsed < BOB_SETTLE_MS
        ? static_cast<float>(elapsed) / BOB_SETTLE_MS : 1.0f;
    const float blend = progress * progress * (3.0f - 2.0f * progress);
    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        // Ease into a common stance before starting the synchronized movement.
        moveServo(leg, 0, lerp(bobStartAngles[leg].x, target.x, blend));
        moveServo(leg, 1, lerp(bobStartAngles[leg].y, target.y, blend));
        moveServo(leg, 2, lerp(bobStartAngles[leg].z, target.z, blend));
    }
}

void walk(float theta) {
    
    calcPoints(startingPoints, endPoints, theta);

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        calcCurrentPoint(
            startingPoints[leg], endPoints[leg],
            current_point_line[leg], current_point_bezier[leg],
            t
        );
        calcCurrentAngles(
            current_point_line[leg], current_point_bezier[leg],
            angles_line[leg], angles_bezier[leg],
            t
        );
    }

    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        if (cycleCtr % 2 == 0) {
            if (leg % 2 == 0) {
                moveServo(leg, 0, angles_line[leg].x);
                moveServo(leg, 1, angles_line[leg].y);
                moveServo(leg, 2, angles_line[leg].z);
            } else {
                moveServo(leg, 0, angles_bezier[leg].x);
                moveServo(leg, 1, angles_bezier[leg].y);
                moveServo(leg, 2, angles_bezier[leg].z);
            }
        } else {
            if (leg % 2 == 0) {
                moveServo(leg, 0, angles_bezier[leg].x);
                moveServo(leg, 1, angles_bezier[leg].y);
                moveServo(leg, 2, angles_bezier[leg].z);
            } else {
                moveServo(leg, 0, angles_line[leg].x);
                moveServo(leg, 1, angles_line[leg].y);
                moveServo(leg, 2, angles_line[leg].z);
            }
        }
    }
    delay(10);
    
    
    t += v;
    
    if (t >= 1.0f) {
        t = 0.0f;
        cycleCtr++;
    }
}

void calcCurrentPoint(vec3 &startingPoint, vec3 &endPoint, vec3 &current_point_line, vec3 &current_point_bezier, float t) {
    // Aktuelle Position auf der Linie zwischen Punkt 1 und Punkt 2 berechnen
    current_point_line.x = lerp(startingPoint.x, endPoint.x, t);
    current_point_line.y = lerp(startingPoint.y, endPoint.y, t);
    current_point_line.z = lerp(startingPoint.z, endPoint.z, t);
    
    current_point_bezier.x = lerp(endPoint.x, startingPoint.x, t);
    current_point_bezier.y = lerp(0.0f, -60.0f, 2*t);
    current_point_bezier.z = lerp(endPoint.z, startingPoint.z, t);
}

void calcCurrentAngles(vec3 &current_point_line, vec3 &current_point_bezier, vec3 &angles_line, vec3 &angles_bezier, float t) {
    angles_line = pointToServoAngles(current_point_line);
    angles_bezier = pointToServoAngles(current_point_bezier);
}

vec3 pointToServoAngles(vec3 point) {
    vec3 angles = pointToAngles(point);

    // Apply the same mechanical offsets and servo ranges to every movement.
    angles.y = 180.0f - angles.y - 27.7;
    angles.z = 180.0f - angles.z + 27.7;
    angles.x = COXA_MIN + angles.x * (COXA_MAX - COXA_MIN) / 90.0f;
    angles.y = FEMUR_MIN + angles.y * (FEMUR_MAX - FEMUR_MIN) / 180.0f;
    angles.z = TIBIA_MIN + angles.z * (TIBIA_MAX - TIBIA_MIN) / 180.0f;
    return angles;
}

// Berechnung der Start- und Endpunkte für die Bewegung jedes Beins
void calcPoints(vec3 p1 [], vec3 p2 [], float theta) {
    float stepDistance = 35.0f;
    for (int leg = 0; leg < NUM_LEGS; leg++) {

        // 60 => Abstand zwischen den Beinen, 45 => Offset durch 90° DOF
        float deltaX = sin(toRad((-60 * leg) - (45 + theta))) * stepDistance;
        float deltaZ = cos(toRad((-60 * leg) - (45 + theta))) * stepDistance;


        p1[leg].x = stepDistance - deltaX;
        p2[leg].x = stepDistance + deltaX;

        p1[leg].y = 0.0f;
        p2[leg].y = 0.0f;

        p1[leg].z = stepDistance - deltaZ;
        p2[leg].z = stepDistance + deltaZ;

        Serial.print("Leg ");        Serial.print(leg);
        Serial.print("Point 1: ("); Serial.print(p1[leg].x); Serial.print(", "); Serial.print(p1[leg].y); Serial.print(", "); Serial.print(p1[leg].z); Serial.println(")");
        Serial.print("Leg ");        Serial.print(leg);
        Serial.print("Point 2: ("); Serial.print(p2[leg].x); Serial.print(", "); Serial.print(p2[leg].y); Serial.print(", "); Serial.print(p2[leg].z); Serial.println(")");
    }
}

// Umrechnung von Grad in Radiant
float toRad(float deg) {
    return deg * PI / 180.0f;
}

// Lineare Interpolation, bei werten >1 wird die Richtung umgekehrt, damit die Bewegung zurück zum Startpunkt erfolgt
float lerp(float p1, float p2, float t) {
    return t <= 1 ? (p1 * (1 - t) + p2 * t) : (p2 * (2 - t) + p1 * (t - 1));
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
