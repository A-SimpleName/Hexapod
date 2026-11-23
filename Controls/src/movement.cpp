#include "movement.h"
#include "servocontrols.h"

namespace {
    constexpr float HEIGHT = 100.0f;
    constexpr float STANCE_MM = 35.0f;
    constexpr unsigned long SERVO_FRAME_MS = 20;

    // One step exchanges the supporting and swinging tripods.
    constexpr float WALK_STRIDE_MM = 40.0f;
    constexpr float WALK_LIFT_MM = 22.0f;
    constexpr unsigned long WALK_STEP_MS = 560;
    constexpr unsigned long WALK_SETTLE_MS = 800;
    constexpr float WALK_STEERING_TIME_MS = 180.0f;

    struct Direction {
        float x;
        float z;
    };

    unsigned long walkStageStartedAtMs = 0;
    unsigned long walkLastFrameMs = 0;
    bool walkSettling = false;
    Direction walkDirection;
    vec3 walkStartAngles[NUM_LEGS];

    constexpr float BOB_AMPLITUDE_MM = 10.0f;
    constexpr unsigned long BOB_PERIOD_MS = 2400;
    constexpr unsigned long BOB_SETTLE_MS = 1000;

    unsigned long bobStartedAtMs = 0;
    unsigned long bobLastFrameMs = 0;
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

    float lerp(float start, float end, float progress) {
        return start * (1.0f - progress) + end * progress;
    }

    float lastAngleOr(int leg, int joint, float fallback) {
        const int angle = getLastServoAngle(leg, joint);
        return angle >= 0 ? angle : fallback;
    }

    void captureStartingPose(vec3 angles[NUM_LEGS]) {
        const vec3 neutral = pointToServoAngles({STANCE_MM, 0.0f, STANCE_MM});
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
}

void startBobUpDown() {
    bobStartedAtMs = millis();
    bobLastFrameMs = bobStartedAtMs - SERVO_FRAME_MS;
    captureStartingPose(bobStartAngles);
}

void bobUpDown() {
    const unsigned long now = millis();
    if (now - bobLastFrameMs < SERVO_FRAME_MS) {
        delay(1);
        return;
    }
    bobLastFrameMs = now;

    const unsigned long elapsed = now - bobStartedAtMs;
    vec3 foot = {STANCE_MM, 0.0f, STANCE_MM};
    if (elapsed >= BOB_SETTLE_MS) {
        const float phase = 2.0f * PI *
            ((elapsed - BOB_SETTLE_MS) % BOB_PERIOD_MS) / BOB_PERIOD_MS;
        // Keep each foot's horizontal position fixed; body height varies from 80 to 100 mm.
        foot.y = BOB_AMPLITUDE_MM * (cosf(phase) - 1.0f);
    }
    const float progress = elapsed < BOB_SETTLE_MS
        ? static_cast<float>(elapsed) / BOB_SETTLE_MS : 1.0f;
    blendToPose(bobStartAngles, pointToServoAngles(foot), smoothstep(progress));
}

void startWalking() {
    walkStageStartedAtMs = millis();
    walkLastFrameMs = walkStageStartedAtMs - SERVO_FRAME_MS;
    walkSettling = true;
    captureStartingPose(walkStartAngles);
}

void walk(float theta) {
    const unsigned long now = millis();
    const unsigned long frameElapsed = now - walkLastFrameMs;
    if (frameElapsed < SERVO_FRAME_MS) {
        delay(1);
        return;
    }
    walkLastFrameMs = now;

    const unsigned long elapsed = now - walkStageStartedAtMs;
    const Direction requested = directionForTheta(theta);
    if (walkSettling) {
        const float progress = elapsed < WALK_SETTLE_MS
            ? static_cast<float>(elapsed) / WALK_SETTLE_MS : 1.0f;
        const vec3 neutral = pointToServoAngles({STANCE_MM, 0.0f, STANCE_MM});
        blendToPose(walkStartAngles, neutral, smoothstep(progress));
        if (elapsed >= WALK_SETTLE_MS) {
            walkSettling = false;
            walkStageStartedAtMs = now;
            walkDirection = requested;
        }
        return;
    }

    // Blend direction vectors so reversals slow through zero and angles wrap naturally.
    const float steeringBlend = 1.0f - expf(-static_cast<float>(frameElapsed) / WALK_STEERING_TIME_MS);
    walkDirection.x = lerp(walkDirection.x, requested.x, steeringBlend);
    walkDirection.z = lerp(walkDirection.z, requested.z, steeringBlend);

    const unsigned long step = elapsed / WALK_STEP_MS;
    const float phase = static_cast<float>(elapsed % WALK_STEP_MS) / WALK_STEP_MS;
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
        // Preserve the leg mounting angles and joystick convention of the original gait.
        const float mounting = toRad(-60.0f * leg - 45.0f);
        const float sinMount = sinf(mounting);
        const float cosMount = cosf(mounting);
        const float dx = sinMount * walkDirection.z + cosMount * walkDirection.x;
        const float dz = cosMount * walkDirection.z - sinMount * walkDirection.x;
        const vec3 foot = {
            STANCE_MM + offset * dx,
            supporting ? 0.0f : -WALK_LIFT_MM * lift * lift,
            STANCE_MM + offset * dz
        };
        moveLeg(leg, pointToServoAngles(foot));
    }
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
