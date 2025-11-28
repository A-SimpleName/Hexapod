#include "calibrations.h"

// Template für alle 6 Beine – gleiche Offsets/Min/Max
// dir = +1 erstmal überall, Drehrichtung kannst du danach Bein für Bein anpassen.
JointCal CAL[6][3] = {
/* Leg 0 */ {
  /* COXA  (Servo 0) */ { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  /* FEMUR (Servo 1) */ { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  /* TIBIA (Servo 2) */ { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
/* Leg 1 */ {
  { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
/* Leg 2 */ {
  { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
/* Leg 3 */ {
  { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
/* Leg 4 */ {
  { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
/* Leg 5 */ {
  { COXA_OFFSET_ZERO, +1.0f,  COXA_MIN, COXA_MAX,  500, 2500 },
  { FEMUR_OFFSET_ZERO, +1.0f,  FEMUR_MIN, FEMUR_MAX,  500, 2500 },
  { TIBIA_OFFSET_ZERO, +1.0f,  TIBIA_MIN, TIBIA_MAX,  500, 2500 }
},
};

