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
    setupPWM(pwm);        // PWM Boards initialisieren
    delay(500);

    // Alle Beine in "0-Punkt"-Neutralpose fahren
    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        moveServo(leg, 0, 67);  // Coxa 0-Punkt
        moveServo(leg, 1, 21);  // Femur 0-Punkt
        moveServo(leg, 2, 24);  // Tibia 0-Punkt
        delay(50);
    }

    Serial.println("Alle Beine in Neutralpose.");
}

void loop() {
    
}





