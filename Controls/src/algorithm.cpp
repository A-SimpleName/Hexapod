#include "algorithm.h"
#include "calibrations.h"
#include "servocontrols.h"

float defaultWaveLegOffsets[NUM_LEGS] = {0, 57.6, 115.2, 172.8, 230.4, 288.0};  

void waveLegs(float legOffsets[], float speed) {
    static float t = 0.0f;  // time or phase
    float mid = (FEMUR_MIN + FEMUR_MAX) / 2.0f;
    float amplitude = (FEMUR_MAX - FEMUR_MIN) / 2.0f;

    for (uint16_t i = 0; i < NUM_LEGS; i++) {
        // Convert offset to radians for sine
        float phase = legOffsets[i] * (PI / 180.0f);
        femur[i].angle = mid + amplitude * sin(t + phase);

        moveServo(i, 1, femur[i].angle);
    }

    t += speed;  // increment time, controls speed
    delay(30);  // adjust delay as needed
}

static void sweepJoint(int leg, Joint joint, JointState& state, float speed, int minAngle, int maxAngle) {
    moveServo(leg, joint, state.angle);
    state.angle += state.dir * speed;
    if (state.angle <= minAngle || state.angle >= maxAngle) {
        state.dir = -state.dir;
    }
}

void circleJerk(float speed) {
    for (int i = 0; i < NUM_LEGS; i++) {
        sweepJoint(i, COXA, coxa[i], speed, COXA_MIN, COXA_MAX);
        sweepJoint(i, FEMUR, femur[i], speed, FEMUR_MIN, FEMUR_MAX);
        sweepJoint(i, TIBIA, tibia[i], speed, TIBIA_MIN, TIBIA_MAX);
    }
    delay(7);
}
