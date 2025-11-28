#include "algorithm.h"
#include "calibrations.h"
#include "servocontrols.h"



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


void circleJerk() {
    for (int i = 0; i < NUM_LEGS; i++) {
        moveServo(i, 0, angle_coxa);

        moveServo(i, 1, angle_femur);

        moveServo(i, 2, angle_tibia);
        
    }

    angle_coxa += dir_coxa * 2;
    angle_femur += dir_femur * 2;
    angle_tibia += dir_tibia * 2;

    if (angle_femur >= FEMUR_MAX || angle_femur <= FEMUR_MIN) {
        dir_femur = -dir_femur;
    }

    if (angle_tibia >= TIBIA_MAX || angle_tibia <= TIBIA_MIN) {
        dir_tibia = -dir_tibia;
    }

    if (angle_coxa >= COXA_MAX || angle_coxa <= COXA_MIN) {
        dir_coxa = -dir_coxa;
    }



    delay(10);
}