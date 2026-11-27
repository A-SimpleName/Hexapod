# Hexapod ESP32 firmware

The firmware runs on a LOLIN32 ESP32 using Arduino and PlatformIO. It creates the robot's Wi-Fi access point, receives UDP commands, and drives 18 servos through two PCA9685 PWM boards.

## Build and inspect

From this directory, use PlatformIO:

```sh
pio run -e lolin32
pio run -e lolin32 -t upload
pio device monitor -b 115200
```

Connect the board by USB for upload and serial monitoring. [platformio.ini](platformio.ini) defines the board, framework, and dependencies. Optionally put machine-specific serial port overrides in `platformio.local.ini`.

## Code map

| File | Responsibility |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Logs the boot reset reason, starts Wi-Fi before initializing PWM and joint state, then polls UDP and advances the active motion in the main loop. |
| [src/hexapod_server.cpp](src/hexapod_server.cpp) | Configures the access point, starts UDP, reads commands, rejects stale packets, and dispatches actions. |
| [src/movement.cpp](src/movement.cpp) and [include/movement.h](include/movement.h) | Define gait timing, foot trajectories, Bob motion, and inverse kinematics, which converts foot targets into joint angles. |
| [src/algorithm.cpp](src/algorithm.cpp) | Implements the Wave Legs and Circle Jerk joint sweeps. |
| [src/servocontrols.cpp](src/servocontrols.cpp) | Maps each leg and joint to a PCA9685 board and channel, clamps angles, and writes pulses at 50 Hz. The boards use I²C addresses `0x40` and `0x41`. |
| [include/calibrations.h](include/calibrations.h) and [src/calibrations.cpp](src/calibrations.cpp) | Define shared calibration constants and the per-joint `CAL` table. The active movement path uses the joint limit constants and the angle conversion in `movement.cpp`. The `CAL` table currently has no reader. |

For walking and Bob, motion code updates joint targets about every 20 ms. [servocontrols.cpp](src/servocontrols.cpp) remembers the last commanded angles so the next motion can blend from that pose. The stored angles are commands sent to the servos. The firmware has no position feedback. The files in [data/](data/) are a legacy browser controller. Despite its name, `setupWebServer()` starts the UDP listener. The current firmware has no HTTP handler for the browser controller.

## Movement behavior

- **Tripod** alternates the even- and odd-numbered leg groups. Walking starts with a 400 ms transition to the standing pose, and steering blends between direction vectors.
- **Rotate** uses the same alternating groups and 400 ms startup transition. The stance feet move tangentially around the body to turn in place. On stop, the firmware lowers raised feet, then returns one group at a time to the standing stance.
- **Bipod** uses the [Bipod-B footfall pattern](https://pmc.ncbi.nlm.nih.gov/articles/PMC5321742/#f4) with pairs 0/3, 1/4, and 2/5. The outgoing pair stays grounded through the first 20% of the next support interval. Travel ramps up from centered feet. On stop, the gait lowers swinging feet in place and centers one pair at a time. If still entering the starting pose, it finishes that transition and holds. A requested replacement movement waits for Bipod to finish stopping.
- **Bob up/down** blends into its starting pose over one second. It keeps the horizontal foot positions fixed and varies modeled body height from 80 to 100 mm. Changing the cycle time while Bob is active preserves its phase.

Physical travel and balance depend on the robot's calibration. Support the robot and keep its legs clear for the first motion check. The [Flutter app guide](../app/README.md) covers controller settings and Android Wi-Fi.

## UDP protocol

### Connect and send

1. Join the robot's **Hexapod-ESP32** Wi-Fi access point. Look up the configured password in [hexapod_server.cpp](src/hexapod_server.cpp).
2. Send each command as a single UTF-8 JSON object in one UDP datagram to **`192.168.4.1:4210`**. Keep packets at or below 255 bytes and use JSON numbers for numeric fields.
3. Include a string `id`. For a controller app, also include a stable integer `session` chosen at startup and an integer `seq` that starts at 1 and increases with every packet, including repeats. Use positive integers that fit in a signed 32-bit value. The ESP32 resets its sequence check when the session changes and drops packets whose sequence is less than or equal to the last processed value. Use one active controller at a time because the firmware tracks one session globally.
4. Treat sends as best effort. The robot sends no acknowledgment or telemetry. Optional `session` and `seq` may be omitted for a simple one-off command.

Example walking packet:

```json
{"id":"move","theta":180,"step_ms":300,"lift_mm":22,"session":12345,"seq":1}
```

### Commands

| `id` | Optional fields | Behavior |
| --- | --- | --- |
| `move` | `theta`, `step_ms`, `lift_mm` | Walk using Tripod. Lift accepts 10–26 mm and defaults to 22 mm. |
| `move_bipod` | `theta`, `step_ms`, `lift_mm` | Walk using Bipod. Lift accepts 6–16 mm and defaults to 14 mm. This command is available in firmware regardless of the Flutter app's local Bipod setting. |
| `rotate` | `direction`, `step_ms` | Turn in place using Tripod. Use `-1` for left (counterclockwise) or `1` for right (clockwise) as viewed from above. Defaults to right. Step time accepts 200–800 ms and defaults to 400 ms. |
| `bob_up_down` | `period_ms` | Repeat the body-height cycle. Accepts 400–4800 ms and defaults to 2000 ms when entering Bob. Omitted or out-of-range values leave the current period in effect while Bob is active. |
| `wave_legs` | | Run the built-in leg wave. The firmware uses its preset even if a client sends `speed` or `legOffsets`. |
| `circle_jerk` | | Run the built-in joint sweep. The firmware uses its preset even if a client sends `speed`. |
| `stop` | | End the active motion and clear queued motion. Bipod and rotation return to the standing pose. Other motions hold their last commanded servo positions. |
| `recover_pwm` | | Maintenance command that clears active and queued motion, reinitializes both PWM boards, and disables the mapped servo outputs. |

For both walking commands, send the same gait ID about every **50 ms** while steering. Send a finite direction angle in degrees as `theta`. The Flutter joystick maps down to 0° (also 360°), left to 90°, up to 180°, and right to 270°. An omitted angle keeps the last direction, initially 0°. `step_ms` is the time per gait step, with an accepted range of **100–800 ms** and a default of **300 ms**. Smaller values step faster. The firmware picks step time and lift when the gait starts, and updates the angle on each packet. Stop the gait and start it again to apply different timing or lift. Missing or out-of-range step time and lift use their respective defaults on the next start.

To start a left turn, send `{"id":"rotate","direction":-1,"step_ms":400}` once. Shorter steps turn faster. The physical turn rate depends on the robot. The robot keeps turning until it receives Stop or another movement. A change in direction or step time goes through the standing stance before the next turn starts.

Bipod starts its stopping sequence after **250 ms** without a fresh Bipod command. Tripod and the repeating motions keep running until a Stop or replacement command arrives. Send Stop when motion input is released or before disconnecting. The Flutter app repeats Stop three times with 30 ms gaps on joystick release to improve delivery over UDP.

### Example Python controller

This example walks in one direction for roughly two seconds, then sends Stop. It uses only Python's standard library. Connect the computer to the robot's Wi-Fi and support the robot before running the example.

```python
import json
import random
import socket
import time

ROBOT = ("192.168.4.1", 4210)

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
    session = random.randint(1, 2**31 - 1)
    sequence = 0

    def send(command_id, **fields):
        global sequence
        sequence += 1
        packet = {"id": command_id, "session": session, "seq": sequence, **fields}
        udp.sendto(json.dumps(packet).encode("utf-8"), ROBOT)

    try:
        until = time.monotonic() + 2
        while time.monotonic() < until:
            send("move", theta=180, step_ms=300, lift_mm=22)
            time.sleep(0.05)
    finally:
        for attempt in range(3):
            if attempt:
                time.sleep(0.03)
            send("stop")
```
