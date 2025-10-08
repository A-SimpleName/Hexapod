#ifndef ALGORITHM_H
#define ALGORITHM_H

#define l1 3.5
#define height 11.0
#define DEFAULT_STEP_HEIGHT 5.0


struct Point {
    double x;
    double y;
};

struct Point3d {
    double x;
    double y;
    double z;
};

extern double t;
extern int legAngles [6][3];
extern Point3d lastPoints [6];
extern Point3d endPoint [6];    // endpunkte für Bewegung
extern Point3d startPoint [6];  // anfangspunkte für Bewegung (alle vorne, nicht extra abgewechselt)
extern bool walking;
extern bool walkingDirection;   // true -> forwärts | false -> rückwärts
extern bool rotating;
extern bool rotatingDirection;  // true -> im Uhrzeigersinn
extern bool wasWalkingForward;  // Hilfsvariable für Richtungsäderung
extern bool legGroup1;          // hilfsvariable für wechsel der sich bewegenden Beine

void walk();
void rotate();
void pointToAngles(Point3d p, int leg);
Point3d getPointLine(Point3d p1, Point3d p2);
Point3d getPointBezier(Point3d P1, Point3d P2, double stepHeight);
Point3d convertTo3D(Point3d p1_3d, Point3d p2_3d, Point p1_2d, Point p2_2d, Point q_2d);
Point lerp(Point p1, Point p2);
void resetPosition(bool tmp);

#endif