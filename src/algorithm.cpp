#include "algorithm.h"
#include "servocontrols.h"

Kinematics legs[6] = {
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3)
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

void waveLegs() {
  float angle = 0;
  float servoAngle = 0;
  for (int j = 0; j < 5; j++) {  
    for (uint16_t i = 0; i < 6; i++) {
      servoAngle = angle + (165.0f / 6.0f * i);
      moveServo(i, 1, (angle > 165.0f) ? 165.0f - (angle - 144.0f) : angle);
    }
    angle += 1.0f;
    angle = fmod((angle - 21.0f), 288.0f) + 21.0f;
    // angle > 165 || angle < 21;
    delay(15);
  }
}

void circleJerk() {
  float lbCoxa = 67;
  float lbFemur = 21;
  float lbTibia = 24;

  float ubCoxa = 142;
  float ubFemur = 165;
  float ubTibia = 186;

  for (;;) {  
    for (uint16_t i = 0; i < 6; i++) {
      
    }
    delay(15);
  }
}