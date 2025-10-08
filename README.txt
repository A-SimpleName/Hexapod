Bewegungs-Loop
{

    for (int i = 0; i < 6; i++) {

        // auswahl der entsprechenden Beine:
        // da sich die Beine abwechseln muss überprüft werden, welche 3 sich aktuell bewegen müssen, deshalb "&& legGroup1"

        if (i % 2 == 1 && legGroup1) {

            // last Point wird gespeichert weil ka ob ma jetzt auf de Winkel zugreifen kinan

            Point3d lastPoint = getPointLine(startPoint[i], endPoint[i]);
            pointToAngles(lastPoint, i);    // Punkte werden in der Methode in ein globales Array gespeichert
            lastPoints[i] = lastPoint;
        } else {
            Point3d lastPoint = getPointBezier(startPoint[i], endPoint[i], defaultStepHeight);
            pointToAngles(lastPoint, i);
            lastPoints[i] = lastPoint;
        }
        
        // Servos Bewegen

    }
}

Rotation-Loop
{

    while (rotating) {
        for (int i = 0; i < 6; i += 2) {
            if (i % 2 == 1 && legGroup1) {
                Point3d lastPoint = getPointBezier({1.0, height, 5.0}, {5.0, height, 1.0}, defaultStepHeight);
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;
        
                // Servos Bewegen

            } else {
                Point3d lastPoint = getPointLine({5.0, height, 1.0}, {1.0, height, 5.0});
                pointToAngles(lastPoint, i);
                lastPoints[i] = lastPoint;
        
                // Servos Bewegen

            }
        }

        t += 0.05;
        t = fmod(t, 2);
    }
}

convertTo3d:
{
    Abi G:
    {
        Below is a summary of the main variables used in the approach:

        - 3D Reference Points:
        -  P_1_3D (x_1, y_1, z_1) : The first reference point in 3D space.
        -  P_2_3D (x_2, y_2, z_2) : The second reference point in 3D space.

        - 2D Reference Points:
        -  P_1_2D (u_1, v_1) : The corresponding 2D coordinates for the first reference point.
        -  P_2_2D (u_2, v_2) : The corresponding 2D coordinates for the second reference point.

        - Query Point:
        -  Q_2D (u_q, v_q): The 2D point that you want to convert to a 3D point.

        - Relative Position Scalars:
        - (s): Represents the relative position along the 2D u-axis.
        - (t): Represents the relative position along the 2D v-axis.

        - Resulting 3D Query Point:
        -  Q_3D (x_q, y_q, z_q): The calculated 3D point corresponding to (Q_2D), found by interpolating between the 3D reference points using the relative scalars (s) and (t).

        These variables work together to map the position of the 2D query point (Q_2D) into 3D space using the defined correspondence between the two reference points.
    }

    u bzw. v sind die Axen der 2d-Ebene
    P_1_2D und P_2_2D sind NUR Referenzpunkte hund haben keine Relevanz zur Position der Beine
}

Kosinussatz type shi
{
    90 => Offset für motor
    |
    +------+     rad -> grad          distanz von x|z zu OriginX|OriginZ         rad -> grad
           |     +--------+     +---------------------------------------------+  +--+
    beta = 90 + (180 * atan(y / sqrt(pow(originX - x, 2) + pow(originZ - z, 2))) / PI);
    gamma = 180 - (180 * acos((pow(a, 2) + pow(b, 2) - pow(c, 2)) / (2 * a * b)) / PI);
             |
    +--------+
    |
    153 -> Offset für motor

    alpha *= 0.833; => von 0-90 auf 0-75 grad weil der shit is falsch kalibriert

                          mehr offset
                        +-------------+
    legAngles[leg][0] = 90 - alpha + 67;
    legAngles[leg][1] = beta - 27  + 21;
    legAngles[leg][2] = gamma - 27 + 24;
}
