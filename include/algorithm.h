#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <Kinematics.h>
#include "calibrations.h"
#include "servocontrols.h"

extern float defaultWaveLegOffsets[NUM_LEGS];

void circleJerk(float speed = 2.5f); // default speed = 2.5f
void waveLegs(float legOffsets[] = defaultWaveLegOffsets, float speed=0.15f); // default speed = 0.15f
void initLegs();
void walkForward(uint16_t steps);

#endif