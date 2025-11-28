#include "algorithm.h"
#include "calibrations.h"
#include "servocontrols.h"

float defaultWaveLegOffsets[NUM_LEGS] = {0, 57.6, 115.2, 172.8, 230.4, 288.0};  

static inline int mapToUs(int deg, int inMin, int inMax, int outMin, int outMax) {
    if (deg < inMin) deg = inMin;
    if (deg > inMax) deg = inMax;
    long num = (long)(deg - inMin) * (outMax - outMin);
    return outMin + (int)( num / (inMax - inMin) );
}

static inline int angleRadToUs(int leg, int joint, float q_rad) {
    const JointCal& C = CAL[leg][joint];

    float deg      = q_rad * 180.0f / 3.14159265f;
    float servoDeg = C.offset_deg + C.dir * deg;

    return mapToUs((int)servoDeg, C.min_deg, C.max_deg, C.us_min, C.us_max);
}


void initLegs() {
    for (int i = 0; i < 6; i++) {
        // add_joint_axis calls:
        legs[i].add_joint_axis(0, 0, (i % 2 == 0) ? 1 : -1, 0, 0, 0);  // Coxa
        legs[i].add_joint_axis(0, 1, 0, 0, 0, L1);                     // Femur
        legs[i].add_joint_axis(0, 1, 0, L2, 0, L1);                    // Tibia

        legs[i].add_initial_end_effector_pose(
            1, 0, 0,  L1,
            0, 1, 0,   0,
            0, 0, 1, -(L2 + L3),
            0, 0, 0,   1
        );
    }
}



void walkForward(uint16_t steps) {
    for (uint16_t i = 0; i < steps; i++) {
        
    }
}

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

void circleJerk(float speed) {
    for (int i = 0; i < NUM_LEGS; i++) {
        moveServo(i, 0, coxa[i].angle);
        coxa[i].angle += coxa[i].dir * speed;
        if (coxa[i].angle <= COXA_MIN || coxa[i].angle >= COXA_MAX) {
            coxa[i].dir = -coxa[i].dir;
        }

        moveServo(i, 1, femur[i].angle);
        femur[i].angle += femur[i].dir * speed;
        if (femur[i].angle <= FEMUR_MIN || femur[i].angle >= FEMUR_MAX) {
            femur[i].dir = -femur[i].dir;
        }

        moveServo(i, 2, tibia[i].angle);
        tibia[i].angle += tibia[i].dir * speed;
        if (tibia[i].angle <= TIBIA_MIN || tibia[i].angle >= TIBIA_MAX) {
            tibia[i].dir = -tibia[i].dir;
        }

    }
    delay(7);
}