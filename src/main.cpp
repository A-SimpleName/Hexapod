#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "servocontrols.h"

const ServoInfo servoMap[NUM_LEGS][SERVOS_PER_LEG] = {
    {{0, 13}, {0, 14}, {0, 15}},
    {{0, 5},  {0, 6},  {0, 7}},
    {{0, 0},  {0, 1},  {0, 2}},
    {{1, 13}, {1, 14}, {1, 15}},
    {{1, 8},  {1, 9},  {1, 10}},
    {{1, 0},  {1, 1},  {1, 2}},
};

void moveAllLegsNeutral() {
    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        moveServo(leg, 0, 67); // Coxa
        moveServo(leg, 1, 21); // Femur
        moveServo(leg, 2, 24); // Tibia
        delay(50);
    }
}

void scanServos() {
    Serial.println("Starte Servo-Scan...");

    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        for (int s = 0; s < SERVOS_PER_LEG; ++s) {
            Serial.print("Leg ");
            Serial.print(leg);
            Serial.print(", Servo ");
            Serial.print(s);
            Serial.println(" -> 90°");

            moveServo(leg, s, 90);
            delay(800);

            Serial.println(" -> zurück auf 67/21/24");
            if (s == 0) moveServo(leg, s, 67);
            if (s == 1) moveServo(leg, s, 21);
            if (s == 2) moveServo(leg, s, 24);
            delay(500);
        }
    }

    Serial.println("Servo-Scan fertig.");
}


void setup() {
    Serial.begin(115200);
    setupPWM(pwm);
    delay(500);

    

}


void loop() {
    static int angle = 21;
    static int dir   = +1;

    
    for (uint16_t i = 0; i < NUM_LEGS; i++) {
        angle += dir * 2;

        moveServo(i, 1, angle); // Bein 0, Femur beugen/strecken
    }
    
    if (angle > 165 || angle < 21) dir *= -1;
    delay(20);
}

