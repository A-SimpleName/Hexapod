#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include "servocontrols.h"
#include "algorithm.h"
#include "movement.h"



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


/*
    ToDo: Extra Methoden erstellen, damit die Logik in loop() übersichtlicher wird
*/

void loop() {
    static float t = 0.0f;
    static float v = 0.2f;
    static int cycleCtr = 0;
    static float theta = 0.0f;
    
    static vec3 startingPoints [NUM_LEGS];
    static vec3 endPoints [NUM_LEGS];

    calcPoints(startingPoints, endPoints, theta);

    static vec3 current_point_line [NUM_LEGS];
    static vec3 current_point_bezier [NUM_LEGS];

    static vec3 angles_line [NUM_LEGS];
    static vec3 angles_bezier [NUM_LEGS];

    for (int leg = 0; leg < NUM_LEGS; leg++) {
        calcCurrentPoint(
            startingPoints[leg], endPoints[leg],
            current_point_line[leg], current_point_bezier[leg],
            t
        );
        calcCurrentAngles(
            current_point_line[leg], current_point_bezier[leg],
            angles_line[leg], angles_bezier[leg],
            t
        );
    }

    for (int leg = 0; leg < NUM_LEGS; ++leg) {
        if (cycleCtr % 2 == 0) {
            if (leg % 2 == 0) {
                moveServo(leg, 0, angles_line[leg].x);
                moveServo(leg, 1, angles_line[leg].y);
                moveServo(leg, 2, angles_line[leg].z);
            } else {
                moveServo(leg, 0, angles_bezier[leg].x);
                moveServo(leg, 1, angles_bezier[leg].y);
                moveServo(leg, 2, angles_bezier[leg].z);
            }
        } else {
            if (leg % 2 == 0) {
                moveServo(leg, 0, angles_bezier[leg].x);
                moveServo(leg, 1, angles_bezier[leg].y);
                moveServo(leg, 2, angles_bezier[leg].z);
            } else {
                moveServo(leg, 0, angles_line[leg].x);
                moveServo(leg, 1, angles_line[leg].y);
                moveServo(leg, 2, angles_line[leg].z);
            }
        }
    }
    delay(10);
    
    
    t += v;
    
    if (t >= 1.0f) {
        t = 0.0f;
        theta += 15.0f;
        cycleCtr++;
    }
}

void calcCurrentPoint(vec3 &startingPoint, vec3 &endPoint, vec3 &current_point_line, vec3 &current_point_bezier, float t) {
    // Aktuelle Position auf der Linie zwischen Punkt 1 und Punkt 2 berechnen
    current_point_line.x = lerp(startingPoint.x, endPoint.x, t);
    current_point_line.y = lerp(startingPoint.y, endPoint.y, t);
    current_point_line.z = lerp(startingPoint.z, endPoint.z, t);
    
    current_point_bezier.x = lerp(endPoint.x, startingPoint.x, t);
    current_point_bezier.y = lerp(0.0f, -60.0f, 2*t);
    current_point_bezier.z = lerp(endPoint.z, startingPoint.z, t);
}

void calcCurrentAngles(vec3 &current_point_line, vec3 &current_point_bezier, vec3 &angles_line, vec3 &angles_bezier, float t) {
    // Berechnung der Winkel für den zuvor berechneten Punkt
    angles_line = pointToAngles(current_point_line);
    angles_bezier = pointToAngles(current_point_bezier);

    // Winkel auf servo limits anpassen

    /*
        ToDo: Magic numbers entfernen
    */
    
    // 27.7 => Offset am 2. Servo
    angles_line.y = 180.0f - angles_line.y - 27.7;
    angles_line.z = 180.0f - angles_line.z + 27.7;

    angles_bezier.y = 180.0f - angles_bezier.y - 27.7;
    angles_bezier.z = 180.0f - angles_bezier.z + 27.7;

    // 90 bzw. 180 => Range of motion pro Servo
    angles_line.x = COXA_MIN + angles_line.x * (COXA_MAX - COXA_MIN) / 90.0f;
    angles_line.y = FEMUR_MIN + angles_line.y * (FEMUR_MAX - FEMUR_MIN) / 180.0f;
    angles_line.z = TIBIA_MIN + angles_line.z * (TIBIA_MAX - TIBIA_MIN) / 180.0f;

    angles_bezier.x = COXA_MIN + angles_bezier.x * (COXA_MAX - COXA_MIN) / 90.0f;
    angles_bezier.y = FEMUR_MIN + angles_bezier.y * (FEMUR_MAX - FEMUR_MIN) / 180.0f;
    angles_bezier.z = TIBIA_MIN + angles_bezier.z * (TIBIA_MAX - TIBIA_MIN) / 180.0f;
}

// Berechnung der Start- und Endpunkte für die Bewegung jedes Beins
void calcPoints(vec3 p1 [], vec3 p2 [], float theta) {
    float stepDistance = 35.0f;
    for (int leg = 0; leg < NUM_LEGS; leg++) {

        // 60 => Abstand zwischen den Beinen, 45 => Offset durch 90° DOF
        float deltaX = sin(toRad((-60 * leg) - (45 + theta))) * stepDistance;
        float deltaZ = cos(toRad((-60 * leg) - (45 + theta))) * stepDistance;


        p1[leg].x = stepDistance - deltaX;
        p2[leg].x = stepDistance + deltaX;

        p1[leg].y = 0.0f;
        p2[leg].y = 0.0f;

        p1[leg].z = stepDistance - deltaZ;
        p2[leg].z = stepDistance + deltaZ;

        Serial.print("Leg ");        Serial.print(leg);
        Serial.print("Point 1: ("); Serial.print(p1[leg].x); Serial.print(", "); Serial.print(p1[leg].y); Serial.print(", "); Serial.print(p1[leg].z); Serial.println(")");
        Serial.print("Leg ");        Serial.print(leg);
        Serial.print("Point 2: ("); Serial.print(p2[leg].x); Serial.print(", "); Serial.print(p2[leg].y); Serial.print(", "); Serial.print(p2[leg].z); Serial.println(")");
    }
}

// Umrechnung von Grad in Radiant
float toRad(float deg) {
    return deg * PI / 180.0f;
}

// Lineare Interpolation, bei werten >1 wird die Richtung umgekehrt, damit die Bewegung zurück zum Startpunkt erfolgt
float lerp(float p1, float p2, float t) {
    return t <= 1 ? (p1 * (1 - t) + p2 * t) : (p2 * (2 - t) + p1 * (t - 1));
}
