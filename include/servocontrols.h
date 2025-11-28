#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "Kinematics.h"
#include "calibrations.h"

#ifndef SERVOCONTROLS_H
#define SERVOCONTROLS_H

#define L1 32.0
#define L2 38.0
#define L3 81.0

#define MAX_ANGLE_SERVO_0 142
#define MAX_ANGLE_SERVO_1 165
#define MAX_ANGLE_SERVO_2 186
#define MIN_ANGLE_SERVO_0 67
#define MIN_ANGLE_SERVO_1 21
#define MIN_ANGLE_SERVO_2 24
#define NUM_LEGS 6
#define SERVOS_PER_LEG 3

struct ServoInfo {
    int board;
    int channel;
};

extern Adafruit_PWMServoDriver pwm[2];
extern Kinematics legs[6];

int angleToPulse(int angle);
void moveServo(int leg, int servo, int angle);
void setupPWM(Adafruit_PWMServoDriver pwm[]);
void writeServosUs(int leg, int joint, int us);

extern const ServoInfo servoMap [NUM_LEGS][SERVOS_PER_LEG];
extern Adafruit_PWMServoDriver pwm[2];

#endif