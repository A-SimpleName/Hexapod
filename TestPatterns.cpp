#include <cmath>
#include <algorithm>

// arduino-cli lib install "Adafruit PWM Servo Driver Library";
// arduino-cli lib install "Servo";
// arduino-cli lib install "Wire";

double t = 0.0;
const double height = 11.0; // Abstand vom Boden in cm
const double l1 = 3.5;      // Länge des oberen Teils eines Beins (brauch i nur für inverse kinematik type shi)
const double PI = 3.14159265358979323846;   // π

const double defaultStepHeight = 5.0;
int legAngles [6][3];
Point3d lastPoints [6];

bool walking;
bool walkingDirection;  // true -> forwärts | false -> rückwärts
bool rotating;
bool rotatingDirection; // true -> im Uhrzeigersinn
bool wasWalkingForward; // Hilfsvariable für Richtungsäderung
bool legGroup1;         // hilfsvariable für wechsel der sich bewegenden Beine

// anfangspunkte für Bewegung (alle vorne, nicht extra abgewechselt)
Point3d startPoint [6] = {
    {5.4,   height, 5.4 },
    {5.0,   height, 1.0 },
    {3.8,   height, 0   },
    {0,     height, 3.8 },
    {1.0,   height, 5.0 },
    {5.4,   height, 5.4 }
};

// endpunkte für Bewegung
Point3d endPoint [6] = {
    {0,     height, 3.8 },
    {1.0,   height, 5.0 },
    {5.4,   height, 5.4 },
    {5.4,   height, 5.4 },
    {5.0,   height, 1.0 },
    {3.8,   height, 0   }
};

struct Point {
    double x;
    double y;
};

struct Point3d {
    double x;
    double y;
    double z;
};

void setup() {

    resetPosition(true);
    wasWalkingForward = true;

}

void loop() {

    bool tempBool = true;

    // wenn sich der bot gerade noch nach vorne bzw. hinten bewegt hat und jetz die richtung gewechselt hat, werden start und end Punkte vertauscht
    // also die bewegung fängt jetzt hinten an und höhrt vorne auf

    if (walkingDirection != wasWalkingForward) {
        for (int i = 0; i < 6; i++) {
            Point3d temp;
            temp = startPoint[i];
            startPoint[i] = endPoint[i];
            endPoint[i] = temp;
        }
        t = 1 - t;
    }

    for (int cycle = 0; cycle < 3; cycle++) {

        while (t <= 1 && tempBool) {
            // erklärung des generellen Walk- und rotation-loops in README.txt

            for (int i = 0; i < 6; i++) {
                if (i % 2 == 1 && legGroup1) {
                    Point3d lastPoint = getPointLine(startPoint[i], endPoint[i]);
                    pointToAngles(lastPoint, i);
                    lastPoints[i] = lastPoint;
                } else {
                    Point3d lastPoint = getPointBezier(startPoint[i], endPoint[i], defaultStepHeight);
                    pointToAngles(lastPoint, i);
                    lastPoints[i] = lastPoint;
                }

                // moveServo (i, 0, legAngles[i][0]);
                // moveServo (i, 1, legAngles[i][1]);
                // moveServo (i, 2, legAngles[i][2]);
            }

            t += 0.05;
            // delay(25);

            if (t >= 0.7 && cycle == 2) {
                tempBool = false;
            }
        }
        t = 0;
        legGroup1 = !legGroup1;
    }
    walkingDirection = !walkingDirection;
}


// berechnung für die bewegung als linie (Beine die bei der Bewegung den Boden Berühren)
Point3d getPointLine(Point3d p1, Point3d p2) {

    // Umwandlung in 2d Punkt da Höhe irrelevant ist
    Point p1_2d = {p1.x, p1.z};
    Point p2_2d = {p2.x, p2.z};

    Point p2d = lerp(p1_2d, p2_2d);
    return {p2d.x, height, p2d.y};
}

// Mit, höhe anfangs- und end-Punkt eine Bezier Kurve erstellen (für Step(-sis))
Point3d getPointBezier(Point3d P1, Point3d P2, double stepHeight) {
    Point p1;
    Point p2;
    Point p3;

    // bei einer Bezier Kurve mit anfangs und endpunt auf derselben höhe ist die höhe der kurve = höhe des mittelpunkts / 2
    // weil i a foipfosten bin und de höhe von oben noch unten gmocht hub muas i de step height von da körperhöhe abziehen
    stepHeight = height - (stepHeight * 2);

    p1 = {0, height};
    p3 = {1, height};
    p2 = {(p1.x + p3.x) / 2, stepHeight};

    Point pl1 = lerp(p1, p2);
    Point pl2 = lerp(p2, p3);
    Point p2d = lerp(pl1, pl2);

    return convertTo3D(P1, P2, p1, p3, p2d);
}

// name ist selbsterklärend, abi g erklärung in README.txt
Point3d convertTo3D(Point3d p1_3d, Point3d p2_3d, Point p1_2d, Point p2_2d, Point q_2d) {
    double s = (q_2d.x - p1_2d.x) / (p2_2d.x - p1_2d.x);
    double t = (q_2d.y - p1_2d.y) / (p2_2d.y - p1_2d.y);

    Point3d q_3d;
    q_3d.x = p1_3d.x + s * (p2_3d.x - p1_3d.x);
    q_3d.y = p1_3d.y + t * (p2_3d.y - p1_3d.y);
    q_3d.z = p1_3d.z + s * (p2_3d.z - p1_3d.z);

    return q_3d;
}

// GOAT-Methode
Point lerp(Point p1, Point p2) {
    double xLerp;
    double yLerp;

    xLerp = (1-t) * p1.x + t * p2.x;
    yLerp = (1-t) * p1.y + t * p2.y;

    return {xLerp, yLerp};
}

// Erklärung in README.txt
void rotate() {

    resetPosition(false);

    while (rotating) {
        for (int i = 0; i < 6; i += 2) {
            if (i % 2 == 1 && legGroup1) {
                Point3d lastPoint = getPointBezier({1.0, height, 5.0}, {5.0, height, 1.0}, defaultStepHeight);
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;

                //servo i1 = legAngles[i][0];
                //servo i2 = legAngles[i][1];
                //servo i3 = legAngles[i][2];

            } else {
                Point3d lastPoint = getPointLine({5.0, height, 1.0}, {1.0, height, 5.0});
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;

                //servo i1 = legAngles[i][0];
                //servo i2 = legAngles[i][1];
                //servo i3 = legAngles[i][2];
            }
        }

        t += 0.05;
        if (t > 1) {
            t = 0;
    	    legGroup1 = !legGroup1;
        }
    }
}

// true  => wird auf Wlak-Cycle zurückgesetzt
// false => wird auf Rotation-Cycle zurückgesetzt
void resetPosition(bool tmp) {

    // Kopieren der Letzten Position da diese während der Bewegung abgeändert wird

    Point3d lastPointCopy [6];
    for (int i = 0; i < 6; i++) {
        lastPointCopy[i] = lastPoints[i];
    }

    if (tmp) {
        t = 0;
        while (t <= 1) {
            int ctr = 0;
            while (ctr < 2) {
                for (int i = ctr; i < 6; i += 2) {
                    Point3d lastPoint = getPointBezier(lastPointCopy[i], startPoint[i], defaultStepHeight);
                    pointToAngles(lastPoint, i);
                    lastPoints[i] = lastPoint;

                    //servo i1 = legAngles[i][0];
                    //servo i2 = legAngles[i][1];
                    //servo i3 = legAngles[i][2];

                    t += 0.2;
                }
            }
        }
    } else {
        t = 0;
        while (t <= 1) {
            for (int i = 0; i < 6; i += 2) {
                Point3d lastPoint = getPointBezier(lastPointCopy[i], {5.0, height, 1.0}, defaultStepHeight);
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;

                //servo i1 = legAngles[i][0];
                //servo i2 = legAngles[i][1];
                //servo i3 = legAngles[i][2];

                t += 0.2;
            }
        }

        t = 0;
        while (t <= 1) {
            for (int i = 1; i < 6; i += 2) {
                Point3d lastPoint = getPointBezier(lastPointCopy[i], {1.0, height, 5.0}, defaultStepHeight);
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;

                //servo i1 = legAngles[i][0];
                //servo i2 = legAngles[i][1];
                //servo i3 = legAngles[i][2];

                t += 0.2;
            }
        }
        t = 0;
    }
}

// Wnadelt einen 3d Punkt in Winkel um und speichert diese in ein globales Array
void pointToAngles(Point3d p, int leg) {

    // Punkt auf eigene Variablen umwandeln
    double x = p.x;
    double y = p.y;
    double z = p.z;

    // Da sich Motor A sich auf einer anderen Achse bewegt als die Anderen, müssen die Ursprungspunkte neu berechnet werden.
    double originX;
    double originZ;

    double alpha;
    double beta;
    double gamma;

    // Inverse Kinematok type shi (aus den Beinen B und C wird ein Dreieck gebildet, a und b sind abgemessene Längen der Beine, c ist die dritte Seite)
    double a = 4.16;
    double b = 8.2;
    double c;

    // arkustangens mei goat
    alpha = atan(z/x);

    // Ursprungspunkte werden mit l1 (Teil A des Beins)
    originX = l1 * cos(alpha);
    originZ = l1 * sin(alpha);
    
    // umwandeln in Grad
    alpha = alpha * 180 / PI;

    // satz des Pythagoras => Länge der Fehlenden Dreiecksseite
    c = sqrt(pow(originX - x, 2) + pow(originZ - z, 2) + pow(y, 2));

    // Kosinussastz type shit, rest in README.txt
    beta = 90 + (180 * atan(y / sqrt(pow(originX - x, 2) + pow(originZ - z, 2))) / PI);
    gamma = 180 - (180 * acos((pow(a, 2) + pow(b, 2) - pow(c, 2)) / (2 * a * b)) / PI);

    alpha *= 0.833;

    legAngles[leg][0] = 75 - alpha + 67;
    legAngles[leg][1] = beta - 27  + 21;
    legAngles[leg][2] = gamma - 27 + 24;
}
