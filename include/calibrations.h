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