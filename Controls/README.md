# Movement patterns

Use **Settings → Tripod speed** and **Bipod speed** to save a step time for each
gait. Both range from 300 to 800 ms and default to 450 ms. A previously saved
shared walking speed is used for either gait until you change that gait's speed.
Slide right to speed up at the same stride. The joystick sends the selected
gait's timing as `step_ms` on the next walk.

Each gait also has its own **step height**. Tripod accepts 10–26 mm of lift and
defaults to 22 mm. Bipod accepts 6–16 mm and defaults to 14 mm. The app sends
the chosen height as `lift_mm` independently of speed. The firmware uses the
gait's default for missing or out-of-range heights.

## Bipod gait

Choose **Settings → Walking gait → Bipod**, save, then use the joystick.
Tripod is the default whenever the app opens. Bipod uses the three opposite
leg pairs from the [Bipod-B footfall pattern](https://pmc.ncbi.nlm.nih.gov/articles/PMC5321742/#f4)
and the robot's existing inverse kinematics. The outgoing pair stays down for
the first 20% of the next pair's stance interval. Bipod also ramps up its travel
from centered footholds on the first step.

The UDP command is `{"id":"move_bipod","theta":180,"step_ms":450}`. Keep sending
it while walking. Stop or 250 ms without a fresh command lowers the swinging
feet in place, then brings each pair back to the standing stance in turn.
If the gait is still settling into its starting pose, it finishes settling
and holds that pose. A different movement waits for Bipod to reach the standing
stance. Verify leg pairing with the robot supported before trying it on level
ground. Balance and floor speed depend on the physical robot.

## Bob up/down

Use **Bob up/down** in the Flutter controller to raise and lower the body with
all six legs in phase. The firmware holds the feet at the walking stance's
horizontal midpoint and moves the femur and tibia together using the existing
inverse kinematics and servo limits.

The pattern eases into its stance for one second, then varies the modeled body
height from 80 to 100 mm. In the app, open **Settings → Bob speed** to choose a
cycle time between 1.2 and 4.8 seconds, then press **Bob up/down** to apply it.
The default is 2.4 seconds. `BOB_AMPLITUDE_MM` in `src/movement.cpp` sets half
the total vertical travel, and `BOB_SETTLE_MS` sets the time to enter the stance.
The default cycle time and its limits are in `include/movement.h`.

The UDP command is `{"id":"bob_up_down","period_ms":2400}` on port 4210.
The optional `period_ms` field accepts 1200–4800 milliseconds. Omitting it
uses the default for a new bob or keeps the current speed for an active one.
Changing speed during a bob preserves the phase of the cycle. The pattern
repeats until `{"id":"stop"}` or another movement command arrives. Stop holds
the current Bob pose.

Both the controller app and ESP32 firmware need the update. Physical travel
depends on the robot's calibration and has to be checked on the hardware.
