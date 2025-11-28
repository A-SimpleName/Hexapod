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

#define NUM_LEGS 6
#define SERVOS_PER_LEG 3

struct ServoInfo {
    int board;
    int channel;
};

struct JointState {
    float angle;
    int dir;
};

extern Adafruit_PWMServoDriver pwm[2];
extern Kinematics legs[6];
extern const ServoInfo servoMap [NUM_LEGS][SERVOS_PER_LEG];

extern JointState coxa[NUM_LEGS];
extern JointState femur[NUM_LEGS];
extern JointState tibia[NUM_LEGS];

void initJointStates();
int angleToPulse(int angle);
void moveServo(int leg, int servo, int angle);
void setupPWM(Adafruit_PWMServoDriver pwm[]);
void writeServosUs(int leg, int joint, int us);

#endif