#include "algorithm.h"

Kinematics legs[6] = {
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3)
};

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