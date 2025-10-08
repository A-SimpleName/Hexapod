#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>

#ifndef SERVOCONTROLS_H
#define SERVOCONTROLS_H

struct ServoInfo {
    int board;
    int channel;
};

#define MAX_ANGLE_SERVO_0 142
#define MAX_ANGLE_SERVO_1 165
#define MAX_ANGLE_SERVO_2 186
#define MIN_ANGLE_SERVO_0 67
#define MIN_ANGLE_SERVO_1 21
#define MIN_ANGLE_SERVO_2 24
#define NUM_LEGS 6
#define SERVOS_PER_LEG 3

int angleToPulse(int angle);
void moveServo(int board, int channel, int angle);
void setupPWM(Adafruit_PWMServoDriver pwm[]);

extern const ServoInfo servoMap [NUM_LEGS][SERVOS_PER_LEG];
extern Adafruit_PWMServoDriver pwm[2];

#endif