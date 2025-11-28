#include "servocontrols.h"

float angle_coxa = COXA_MIN;
float angle_femur = FEMUR_MIN;
float angle_tibia = TIBIA_MIN;
int dir_femur = 1;
int dir_coxa = 1;
int dir_tibia = 1;

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
        angle = constrain(angle, MIN_ANGLE_SERVO_1, MAX_ANGLE_SERVO_1);
    } else if (servo == 2) {
        // Servo 2 muss auf 24 Grad gesetzt werden, da dieser auf der anderen Achse arbeitet
        angle = constrain(angle, MIN_ANGLE_SERVO_2, MAX_ANGLE_SERVO_2);
    } else {
        // Servo 0 muss auf 67 Grad gesetzt werden, da dieser auf der anderen Achse arbeitet
        angle = constrain(angle, MIN_ANGLE_SERVO_0, MAX_ANGLE_SERVO_0);
    }
    
    pwm[servoInfo.board].setPWM(servoInfo.channel, 0, angleToPulse(angle));
}

void setupPWM(Adafruit_PWMServoDriver pwm[]) {

    pwm[0].begin();
    pwm[0].setPWMFreq(50); // 50 Hz... Standard frequency for servo motors
    pwm[1].begin();
    pwm[1].setPWMFreq(50);

}