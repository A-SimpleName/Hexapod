#include "movement.h"
#include "servocontrols.h"

namespace {
    constexpr float HEIGHT = 100.0f;
    constexpr float STANCE_MM = 35.0f;
    constexpr vec3 STANDING_FOOT = {STANCE_MM, 0.0f, STANCE_MM};
    constexpr unsigned long SERVO_FRAME_MS = 20;

    // One step exchanges the supporting groups.
    constexpr float WALK_STRIDE_MM = 40.0f;
    constexpr unsigned long WALK_SETTLE_MS = 800;
    constexpr float WALK_STEERING_TIME_MS = 180.0f;
    constexpr unsigned long BIPOD_LAND_MS = 180;
    constexpr unsigned long BIPOD_CENTER_PAIR_MS = 240;
    constexpr int BIPOD_PAIR_COUNT = NUM_LEGS / 2;
    // Each new pair touches down one interval after the last, with 20% overlap.
    constexpr float BIPOD_STANCE_STEPS = 1.2f;
    constexpr float BIPOD_ADVANCE_MM = WALK_STRIDE_MM / BIPOD_STANCE_STEPS;
    constexpr float BIPOD_CENTER_LIFT_MM = 6.0f;

    struct LiftRange {
        long minMm;
        long defaultMm;
        long maxMm;
    };

    constexpr LiftRange TRIPOD_LIFT = {10, 22, 26};
    constexpr LiftRange BIPOD_LIFT = {6, 14, 16};

    struct Direction {
        float x;
        float z;
    };

    enum class WalkStage { Settling, Striding, SettlingToStop, Lowering, Centering, Standing };

    unsigned long walkStageStartedAtMs = 0;
    unsigned long walkLastFrameMs = 0;
    unsigned long walkStepMs = WALK_DEFAULT_STEP_MS;
    float walkLiftMm = TRIPOD_LIFT.defaultMm;
    WalkGait walkGait = WalkGait::Tripod;
    WalkStage walkStage = WalkStage::Standing;
    Direction walkDirection;
    vec3 walkStartAngles[NUM_LEGS];
    vec3 bipodFootTargets[NUM_LEGS];
    int bipodCenterPair = 0;

    constexpr float BOB_AMPLITUDE_MM = 10.0f;
    constexpr unsigned long BOB_SETTLE_MS = 1000;

    unsigned long bobStartedAtMs = 0;
    unsigned long bobLastFrameMs = 0;
    float bobPhase = 0.0f;
    vec3 bobStartAngles[NUM_LEGS];

    float toRad(float degrees) {
        return degrees * PI / 180.0f;
    }

    Direction directionForTheta(float theta) {
        const float radians = toRad(-theta);
        return {sinf(radians), cosf(radians)};
    }

    float smoothstep(float progress) {
        return progress * progress * (3.0f - 2.0f * progress);
    }

    float stageProgress(unsigned long elapsed, unsigned long duration) {
        return min(1.0f, static_cast<float>(elapsed) / duration);
    }

    float lerp(float start, float end, float progress) {
        return start * (1.0f - progress) + end * progress;
    }

    float lastAngleOr(int leg, int joint, float fallback) {
        const int angle = getLastServoAngle(leg, joint);
        return angle >= 0 ? angle : fallback;
    }

    void captureStartingPose(vec3 angles[NUM_LEGS]) {
        const vec3 neutral = pointToServoAngles(STANDING_FOOT);
        for (int leg = 0; leg < NUM_LEGS; ++leg) {
            angles[leg] = {
                lastAngleOr(leg, COXA, neutral.x),
                lastAngleOr(leg, FEMUR, neutral.y),
                lastAngleOr(leg, TIBIA, neutral.z)
            };
        }
    }

    void moveLeg(int leg, vec3 angles) {
        moveServo(leg, COXA, angles.x);
        moveServo(leg, FEMUR, angles.y);
        moveServo(leg, TIBIA, angles.z);
    }

    void blendToPose(const vec3 from[NUM_LEGS], vec3 target, float blend) {
        for (int leg = 0; leg < NUM_LEGS; ++leg) {
            moveLeg(leg, {
                lerp(from[leg].x, target.x, blend),
                lerp(from[leg].y, target.y, blend),
                lerp(from[leg].z, target.z, blend)
            });
        }
    }

    vec3 walkingFoot(int leg, float offset, float lift) {
        // Preserve the leg mounting angles and joystick convention of the tripod gait.
        const float mounting = toRad(-60.0f * leg - 45.0f);
        const float sinMount = sinf(mounting);
        const float cosMount = cosf(mounting);
        const float dx = sinMount * walkDirection.z + cosMount * walkDirection.x;
        const float dz = cosMount * walkDirection.z - sinMount * walkDirection.x;
        return {STANCE_MM + offset * dx, -lift, STANCE_MM + offset * dz};
    }

    float bipodStartupTravel(float timeInSteps) {
        // Integrate a linear ramp to the steady walking speed over the first interval.
        return timeInSteps < 1.0f ? 0.5f * timeInSteps * timeInSteps : timeInSteps - 0.5f;
    }

    void walkBipod(unsigned long step, float phase) {
        const int supportPair = step % BIPOD_PAIR_COUNT;
        const float halfStance = BIPOD_STANCE_STEPS / 2.0f;
        for (int pair = 0; pair < BIPOD_PAIR_COUNT; ++pair) {
            const int stepsSinceStance = (supportPair + BIPOD_PAIR_COUNT - pair) % BIPOD_PAIR_COUNT;
            const float pairPhase = stepsSinceStance + phase;
            float swing = max(0.0f, pairPhase - BIPOD_STANCE_STEPS) / (BIPOD_PAIR_COUNT - BIPOD_STANCE_STEPS);
            // Body travel moves the stance feet together. Swing blends toward the next foothold.
            float offset = pairPhase - halfStance - BIPOD_PAIR_COUNT * smoothstep(swing);
            const int stanceStart = supportPair - stepsSinceStance;
            if (step < BIPOD_PAIR_COUNT && stanceStart <= 0) {
                // The initial foothold is centered. Replant to the normal stride on the first swing.
                const float time = step + phase;
                const float liftOff = max(0.0f, stanceStart + BIPOD_STANCE_STEPS);
                const float touchdown = stanceStart + BIPOD_PAIR_COUNT;
                swing = max(0.0f, time - liftOff) / (touchdown - liftOff);
                offset = bipodStartupTravel(time) - bipodStartupTravel(touchdown + halfStance) * smoothstep(swing);
            }
            const float arc = sinf(PI * swing);
            const float lift = walkLiftMm * arc * arc;
            // Bipod-B pairs opposite legs: 0/3, 1/4, 2/5.
            for (int leg = pair; leg < NUM_LEGS; leg += BIPOD_PAIR_COUNT) {
                const vec3 foot = walkingFoot(leg, BIPOD_ADVANCE_MM * offset, lift);
                bipodFootTargets[leg] = foot;
                moveLeg(leg, pointToServoAngles(foot));
            }
        }
    }

    void lowerBipodFeet(float blend) {
        for (int leg = 0; leg < NUM_LEGS; ++leg) {
            vec3 foot = bipodFootTargets[leg];
            foot.y *= 1.0f - blend;
            moveLeg(leg, pointToServoAngles(foot));
        }
    }

    void centerBipodPair(float blend) {
        // Reposition only this pair. The other four feet keep their grounded targets.
        for (int leg = bipodCenterPair; leg < NUM_LEGS; leg += BIPOD_PAIR_COUNT) {
            const vec3 from = bipodFootTargets[leg];
            const float distance = hypotf(from.x - STANDING_FOOT.x, from.z - STANDING_FOOT.z);
            const float lift = min(BIPOD_CENTER_LIFT_MM, distance) * 4.0f * blend * (1.0f - blend);
            const vec3 foot = {
                lerp(from.x, STANDING_FOOT.x, blend),
                -lift,
                lerp(from.z, STANDING_FOOT.z, blend)
            };
            moveLeg(leg, pointToServoAngles(foot));
        }
    }
}

void startBobUpDown() {
    bobStartedAtMs = millis();
    bobLastFrameMs = bobStartedAtMs - SERVO_FRAME_MS;
    bobPhase = 0.0f;
    captureStartingPose(bobStartAngles);
}

void bobUpDown(unsigned long periodMs) {
    const unsigned long now = millis();
    const unsigned long frameElapsed = now - bobLastFrameMs;
    if (frameElapsed < SERVO_FRAME_MS) {
        delay(1);
        return;
    }
    bobLastFrameMs = now;

    const unsigned long elapsed = now - bobStartedAtMs;
    vec3 foot = STANDING_FOOT;
    if (elapsed >= BOB_SETTLE_MS) {
        const unsigned long sinceSettle = elapsed - BOB_SETTLE_MS;
        const unsigned long motionElapsed = min(frameElapsed, sinceSettle);
        // Accumulate phase so changing period during the motion keeps the current pose.
        bobPhase += static_cast<float>(motionElapsed % periodMs) / periodMs;
        bobPhase -= floorf(bobPhase);
        // Keep each foot's horizontal position fixed; body height varies from 80 to 100 mm.
        foot.y = BOB_AMPLITUDE_MM * (cosf(2.0f * PI * bobPhase) - 1.0f);
    }
    const float progress = stageProgress(elapsed, BOB_SETTLE_MS);
    blendToPose(bobStartAngles, pointToServoAngles(foot), smoothstep(progress));
}

void startWalking(unsigned long stepMs, WalkGait gait, long liftMm) {
    walkStageStartedAtMs = millis();
    walkLastFrameMs = walkStageStartedAtMs - SERVO_FRAME_MS;
    walkStage = WalkStage::Settling;
    walkStepMs = stepMs;
    walkGait = gait;
    const LiftRange range = gait == WalkGait::Bipod ? BIPOD_LIFT : TRIPOD_LIFT;
    walkLiftMm = liftMm >= range.minMm && liftMm <= range.maxMm ? liftMm : range.defaultMm;
    captureStartingPose(walkStartAngles);
}

void startBipodStop() {
    if (walkStage == WalkStage::Settling) {
        walkStage = WalkStage::SettlingToStop;
    } else if (walkStage == WalkStage::Striding) {
        walkStage = WalkStage::Lowering;
        walkStageStartedAtMs = millis();
    }
}

bool walk(float theta) {
    if (walkStage == WalkStage::Standing) {
        return true;
    }
    const unsigned long now = millis();
    const unsigned long frameElapsed = now - walkLastFrameMs;
    if (frameElapsed < SERVO_FRAME_MS) {
        delay(1);
        return false;
    }
    walkLastFrameMs = now;

    const unsigned long elapsed = now - walkStageStartedAtMs;
    if (walkStage == WalkStage::Settling || walkStage == WalkStage::SettlingToStop) {
        const float blend = smoothstep(stageProgress(elapsed, WALK_SETTLE_MS));
        blendToPose(walkStartAngles, pointToServoAngles(STANDING_FOOT), blend);
        if (elapsed >= WALK_SETTLE_MS) {
            if (walkStage == WalkStage::SettlingToStop) {
                walkStage = WalkStage::Standing;
                return true;
            }
            walkStage = WalkStage::Striding;
            walkStageStartedAtMs = now;
            walkDirection = directionForTheta(theta);
            if (walkGait == WalkGait::Bipod) {
                for (vec3& foot : bipodFootTargets) {
                    foot = STANDING_FOOT;
                }
            }
        }
        return false;
    }

    if (walkStage == WalkStage::Lowering) {
        lowerBipodFeet(smoothstep(stageProgress(elapsed, BIPOD_LAND_MS)));
        if (elapsed >= BIPOD_LAND_MS) {
            walkStage = WalkStage::Centering;
            walkStageStartedAtMs = now;
            bipodCenterPair = 0;
        }
        return false;
    }
    if (walkStage == WalkStage::Centering) {
        centerBipodPair(smoothstep(stageProgress(elapsed, BIPOD_CENTER_PAIR_MS)));
        if (elapsed >= BIPOD_CENTER_PAIR_MS) {
            if (++bipodCenterPair == BIPOD_PAIR_COUNT) {
                walkStage = WalkStage::Standing;
                return true;
            }
            walkStageStartedAtMs = now;
        }
        return false;
    }

    // Blend direction vectors so reversals slow through zero and angles wrap naturally.
    const Direction requested = directionForTheta(theta);
    const float steeringBlend = 1.0f - expf(-static_cast<float>(frameElapsed) / WALK_STEERING_TIME_MS);
    walkDirection.x = lerp(walkDirection.x, requested.x, steeringBlend);
    walkDirection.z = lerp(walkDirection.z, requested.z, steeringBlend);

    const unsigned long step = elapsed / walkStepMs;
    const float phase = static_cast<float>(elapsed % walkStepMs) / walkStepMs;
    if (walkGait == WalkGait::Bipod) {
        walkBipod(step, phase);
        return false;
    }
    const float halfStride = WALK_STRIDE_MM / 2.0f;
    const bool firstStep = step == 0;

    // Start centered and reach normal stride speed by the first exchange.
    const float stance = firstStep ? phase * phase : 2.0f * phase - 1.0f;
    // The swing reverses over the step, matching the stance velocity at both ends.
    const float swing = stance + (firstStep ? 0.0f : 2.0f)
        - (firstStep ? 2.0f : 4.0f) * smoothstep(phase);
    const float lift = sinf(PI * phase);

    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        const bool supporting = leg % 2 == step % 2;
        const float offset = halfStride * (supporting ? stance : swing);
        const vec3 foot = walkingFoot(leg, offset, supporting ? 0.0f : walkLiftMm * lift * lift);
        moveLeg(leg, pointToServoAngles(foot));
    }
    return false;
}

vec3 pointToServoAngles(vec3 point) {
    vec3 angles = pointToAngles(point);

    // Apply the same mechanical offsets and servo ranges to every movement.
    angles.y = 180.0f - angles.y - 27.7;
    angles.z = 180.0f - angles.z + 27.7;
    angles.x = COXA_MIN + angles.x * (COXA_MAX - COXA_MIN) / 90.0f;
    angles.y = FEMUR_MIN + angles.y * (FEMUR_MAX - FEMUR_MIN) / 180.0f;
    angles.z = TIBIA_MIN + angles.z * (TIBIA_MAX - TIBIA_MIN) / 180.0f;
    return angles;
}

vec3 pointToAngles(vec3 point) {
    const float radial = sqrtf(point.x * point.x + point.z * point.z) - L1;
    const float height = HEIGHT + point.y;
    const float distanceSquared = radial * radial + height * height;
    const float cosKnee = (L2 * L2 + L3 * L3 - distanceSquared) / (2.0f * L2 * L3);
    const float knee = acosf(constrain(cosKnee, -1.0f, 1.0f));

    // atan2 preserves the obtuse femur triangle angle when the leg folds inward.
    const float femurOffset = atan2f(L3 * sinf(knee), L2 - L3 * cosf(knee));
    const float femur = atan2f(radial, height) + femurOffset;
    return {
        static_cast<float>(atan2f(point.z, point.x) * 180.0f / PI),
        static_cast<float>(femur * 180.0f / PI),
        static_cast<float>(knee * 180.0f / PI)
    };
}
