#ifndef MOVEMENT_H
#define MOVEMENT_H

struct vec3 {
    float x;
    float y;
    float z;
};

constexpr long BOB_DEFAULT_PERIOD_MS = 2400;
constexpr long BOB_MIN_PERIOD_MS = 1200;
constexpr long BOB_MAX_PERIOD_MS = 4800;
constexpr long WALK_DEFAULT_STEP_MS = 450;
constexpr long WALK_MIN_STEP_MS = 300;
constexpr long WALK_MAX_STEP_MS = 800;

enum class WalkGait { Tripod, Bipod };

void startWalking(unsigned long stepMs = WALK_DEFAULT_STEP_MS, WalkGait gait = WalkGait::Tripod,
                  long liftMm = 0);
void walk(float theta);
bool startBipodLanding();
bool landBipod();
void startBobUpDown();
void bobUpDown(unsigned long periodMs = BOB_DEFAULT_PERIOD_MS);
vec3 pointToAngles(vec3 point);
vec3 pointToServoAngles(vec3 point);

#endif
