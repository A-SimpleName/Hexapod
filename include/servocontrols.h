#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "Kinematics.h"
#include "calibrations.h"
#include "movement.h"

#ifndef SERVOCONTROLS_H
#define SERVOCONTROLS_H

#define L1 35.0
#define L2 43.0 // 3.8 | 2
#define L3 82.0

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
extern const ServoInfo servoMap [NUM_LEGS][SERVOS_PER_LEG];

extern float angle_coxa;
extern float angle_femur;
extern float angle_tibia;
extern int dir_femur;
extern int dir_coxa;
extern int dir_tibia;

int angleToPulse(int angle);
void moveServo(int leg, int servo, int angle);
void setupPWM(Adafruit_PWMServoDriver pwm[]);
void writeServosUs(int leg, int joint, int us);
void calcCurrentPoint(vec3 &startingPoint, vec3 &endPoint, vec3 &current_point_line, vec3 &current_point_bezier, float t);
void calcCurrentAngles(vec3 &current_point_line, vec3 &current_point_bezier, vec3 &angles_line, vec3 &angles_bezier, float t);
void calcPoints(vec3 p1 [], vec3 p2 [], float theta);
float toRad(float deg);
float lerp(float p1, float p2, float t);

#endif