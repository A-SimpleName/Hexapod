#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "algorithm.h" 
#include "servocontrols.h"
#include "website.h"

const ServoInfo servoMap[NUM_LEGS][SERVOS_PER_LEG] = {
    {{0, 13}, {0, 14}, {0, 15}},
    {{0, 5}, {0, 6}, {0, 7}},
    {{0, 0}, {0, 1}, {0, 2}},
    {{1, 13}, {1, 14}, {1, 15}},
    {{1, 8}, {1, 9}, {1, 10}},
    {{1, 0}, {1, 1}, {1, 2}},
};

void setup() {
    Serial.begin(115200);
    setupPWM(pwm);    
    moveServo(0, 1, 100);
}

void loop() {
    
}





