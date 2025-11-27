#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <Kinematics.h>

enum Joint { COXA = 0, FEMUR = 1, TIBIA = 2 };

// Kalibrierdaten für EIN Gelenk
struct JointCal {
    float offset_deg;  // mechanischer Nullpunkt
    float dir;         // +1 oder -1 für Invertierung
    float min_deg;     // untere Grenze
    float max_deg;     // obere Grenze
    int us_min;        // minimaler PWM-Wert
    int us_max;        // maximaler PWM-Wert
};

// 6 Beine × 3 Gelenke → Tabelle in algorithm.cpp definiert
extern JointCal CAL[6][3];

#ifndef L1
#define L1 32.0
#endif
#ifndef L2
#define L2 38.0
#endif
#ifndef L3
#define L3 81.0
#endif

#define COXA_OFFSET_ZERO
#define COXA_

extern Kinematics legs[6];

void initLegs();
void walkForward(uint16_t steps);

#endif