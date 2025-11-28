#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "servocontrols.h"
#include "algorithm.h"

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


void setup() {
    Serial.begin(115200);
    setupPWM(pwm);
    delay(500);

    

}


void loop() {

    /*
    for (int i = 0; i < 5; i++) {
        int angle = 21;
        int dir   = +1;
        
        for (uint16_t j = 0;  j < NUM_LEGS; j++) {
            angle += dir * 2;

            moveServo(j, 1, angle); // Bein 0, Femur beugen/strecken
        }
        
        if (angle > 165 || angle < 21) dir *= -1;
        delay(20);
        
    }*/
    waveLegs();
}

