# Movement patterns

Use **Bob up/down** in the Flutter controller to raise and lower the body with
all six legs in phase. The firmware holds the feet at the walking stance's
horizontal midpoint and moves the femur and tibia together using the existing
inverse kinematics and servo limits.

The pattern eases into its stance for one second, then varies the modeled body
height from 80 to 100 mm over a 2.4-second cycle. Tune `BOB_AMPLITUDE_MM`,
`BOB_PERIOD_MS`, and `BOB_SETTLE_MS` in `src/movement.cpp` to change the motion.
`BOB_AMPLITUDE_MM` is half the total vertical travel; the upper height is the
walking code's `HEIGHT`.

The UDP command is `{"id":"bob_up_down"}` on port 4210. It repeats until
`{"id":"stop"}` or another movement command arrives. Stop holds the current
pose, as it does for the other patterns. Repeating the bob command while it is
active does not restart the cycle.

Both the controller app and ESP32 firmware need the update. Physical travel
depends on the robot's calibration and has to be checked on the hardware.
