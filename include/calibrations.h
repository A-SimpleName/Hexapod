#ifndef CALIBRATIONS_H
#define CALIBRATIONS_H

#define COXA_OFFSET_ZERO 104.5f  
#define COXA_MIN 67
#define COXA_MAX 142

#define FEMUR_OFFSET_ZERO 93.0f
#define FEMUR_MIN 21
#define FEMUR_MAX 165

#define TIBIA_OFFSET_ZERO 78.0f
#define TIBIA_MIN 24
#define TIBIA_MAX 186

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

#endif