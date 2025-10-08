// die Bibliothek Servo.h wird eingebunden, sie steuert den Motor
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41);

// minimale bzw. maximale pulslänge
#define SERVOMIN  150
#define SERVOMAX  600

const int leg[7][3] = {
  {0, 0, 0},
  {0, 1, 2},
  {0, 1, 2},
  {4, 5, 6},
  {4, 5, 6},
  {8, 9, 10},
  {8, 9, 10}
};


void setup() {
  Serial.begin(9600);
  pwm1.begin();
  pwm1.setPWMFreq(60);

  pwm2.begin();
  pwm2.setPWMFreq(60);

  //        Board |   Bein   | Position
  for (int i = 1; i < 6; i += 2) {
    for (int j = 0; j < 3; j++) {
      moveServo(pwm1, leg[i][j], 0);
  	  delay(500);
      moveServo(pwm2, leg[i+1][j], 0);
      delay(500);
    }
  }
  
  
}

// moveServo() wird remapped fuer grad statt ticks (von chatgpt kopiert weil juckt)
void moveServo(Adafruit_PWMServoDriver &pwm, int channel, int angle) {
  int pulselen = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulselen);
}

void loop() {
  
}
