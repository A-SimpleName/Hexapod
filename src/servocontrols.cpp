#include "servocontrols.h"

Adafruit_PWMServoDriver pwm[2] = {
    Adafruit_PWMServoDriver(0x40), // Board 0
    Adafruit_PWMServoDriver(0x41)  // Board 1
};

int angleToPulse(int angle) {
    int minPulse = 121;
    int maxPulse = 486;
    return map(angle, 0, 180, minPulse, maxPulse);
}

void moveServo(int leg, int servo, int angle) {
    // Bei Servo 0: 67 und 142 (0-Punkt, links)
    // Bei Servo 1: 21 (0-Punkt, oben) und 165 sind Grenzen, NICHT darüber bzw. darunter gehen, BÖSE!
    // Bei Servo 2: 24 (0-Punkt, oben) und 186
    ServoInfo servoInfo = servoMap[leg][servo];

    Serial.println("Moving servo " + String(servo) + " of leg " + String(leg) + " to angle: " + angle);

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
    Serial.println("Initializing PWMServoDriver...");

    pwm[0].begin();
    pwm[0].setPWMFreq(50); // 50 Hz... Standard frequency for servo motors
    pwm[1].begin();
    pwm[1].setPWMFreq(50);

    Serial.println("PWMServoDriver initialized.");
}