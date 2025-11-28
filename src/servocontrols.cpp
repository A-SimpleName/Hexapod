#include "servocontrols.h"

JointState coxa[NUM_LEGS];
JointState femur[NUM_LEGS];
JointState tibia[NUM_LEGS];

const ServoInfo servoMap[NUM_LEGS][SERVOS_PER_LEG] = {
    {{0, 13}, {0, 14}, {0, 15}},
    {{0, 5},  {0, 6},  {0, 7}},
    {{0, 0},  {0, 1},  {0, 2}},
    {{1, 13}, {1, 14}, {1, 15}},
    {{1, 8},  {1, 9},  {1, 10}},
    {{1, 0},  {1, 1},  {1, 2}},
};

Adafruit_PWMServoDriver pwm[2] = {
    Adafruit_PWMServoDriver(0x40), // Board 0
    Adafruit_PWMServoDriver(0x41)  // Board 1
};

Kinematics legs[6] = {
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3),
    Kinematics(3)
};

void initJointStates() {
    for (int i = 0; i < NUM_LEGS; ++i) {
        coxa[i] = {COXA_MIN, 1};
        femur[i] = {FEMUR_MIN, 1};
        tibia[i] = {TIBIA_MIN, 1};
    }
}

int angleToPulse(int angle) {
    int minPulse = 121;
    int maxPulse = 486;
    return map(angle, 0, 180, minPulse, maxPulse);
}

void moveServo(int leg, int servo, int angle) {
    // Bei Servo 0: 67 (0-Punkt, rechts) und 142 (Oberste)
    // Bei Servo 1: 21 (0-Punkt, oben) und 165 sind Grenzen, NICHT darüber bzw. darunter gehen, BÖSE! (Mitte)
    // Bei Servo 2: 24 (0-Punkt, oben) und 186 (Unten)
    ServoInfo servoInfo = servoMap[leg][servo];

    // Begrenzung der Winkel für die Servos
    if (servo == 1) {
        // Servo 1 muss auf 21 Grad gesetzt werden, da dieser auf der anderen Achse arbeitet
        angle = constrain(angle, FEMUR_MIN, FEMUR_MAX);
    } else if (servo == 2) {
        // Servo 2 muss auf 24 Grad gesetzt werden, da dieser auf der anderen Achse arbeitet
        angle = constrain(angle, TIBIA_MIN, TIBIA_MAX);
    } else {
        // Servo 0 muss auf 67 Grad gesetzt werden, da dieser auf der anderen Achse arbeitet
        angle = constrain(angle, COXA_MIN, COXA_MAX);
    }
    
    pwm[servoInfo.board].setPWM(servoInfo.channel, 0, angleToPulse(angle));
}

void setupPWM(Adafruit_PWMServoDriver pwm[]) {

    pwm[0].begin();
    pwm[0].setPWMFreq(50); // 50 Hz... Standard frequency for servo motors
    pwm[1].begin();
    pwm[1].setPWMFreq(50);

}