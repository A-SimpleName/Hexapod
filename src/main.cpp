#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "servocontrols.h"
#include "algorithm.h"

void setup() {
    Serial.begin(115200);
    setupPWM(pwm);
    delay(500);
    initJointStates();
}

static long startTime = millis();

void loop() {
    if (((millis() - startTime) / 1000) % 8 < 4) {
        waveLegs();
    } else {
        circleJerk();
    }
}


