#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <Kinematics.h>
#include "calibrations.h"

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