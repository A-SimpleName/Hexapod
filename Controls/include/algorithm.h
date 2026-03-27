#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "calibrations.h"
#include "servocontrols.h"

struct GaitParams {
    float strideLengthMm = 60.0f;
    float stepHeightMm   = 25.0f;
    float speedMmPerS    = 80.0f;   // <- “Geschwindigkeit” als Variable
    int ticksPerStep     = 50;      // Auflösung
};

extern float defaultWaveLegOffsets[NUM_LEGS];

void circleJerk(float speed = 2.5f); // default speed = 2.5f
void waveLegs(float legOffsets[] = defaultWaveLegOffsets, float speed = 0.15f); // default speed = 0.15f

#endif