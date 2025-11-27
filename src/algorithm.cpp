#include "algorithm.h"

Kinematics legs[6] = {
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3)
};

#include "algorithm.h"

// Template für alle 6 Beine – gleiche Offsets/Min/Max
// dir = +1 erstmal überall, Drehrichtung kannst du danach Bein für Bein anpassen.
JointCal CAL[6][3] = {
/* Leg 0 */ {
  /* COXA  (Servo 0) */ { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  /* FEMUR (Servo 1) */ { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  /* TIBIA (Servo 2) */ { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
/* Leg 1 */ {
  { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
/* Leg 2 */ {
  { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
/* Leg 3 */ {
  { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
/* Leg 4 */ {
  { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
/* Leg 5 */ {
  { 67.0f, +1.0f,  67.0f, 142.0f,  500, 2500 },
  { 21.0f, +1.0f,  21.0f, 165.0f,  500, 2500 },
  { 24.0f, +1.0f,  24.0f, 186.0f,  500, 2500 }
},
};

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