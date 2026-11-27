# Hexapod controller app

The Flutter app controls the ESP32 hexapod over its local Wi-Fi network. The joystick steers Tripod or Bipod walking, **Bob up/down** starts a repeating body movement, and **Stop** ends the active movement. The [firmware guide and UDP protocol](../Controls/README.md#udp-protocol) describe what the robot does with each command.

## Run and test

Use a Flutter SDK that provides Dart 3.10.1 or a later compatible Dart 3 release. From this directory, with an Android device connected and USB debugging enabled:

```sh
flutter pub get
flutter devices
flutter run -d <device-id>
flutter test
```

On Android 10 or later, use **Connect Wi-Fi** and accept the system permission and network prompts. The app binds its UDP socket to the robot's Wi-Fi network. You can also join **Hexapod-ESP32** in your device's Wi-Fi settings and let the app use its regular UDP socket. The access-point password is configured in [the firmware](../Controls/src/hexapod_server.cpp). Run the controller as a native Flutter app. Its transport uses `dart:io` UDP sockets, which are unavailable in Flutter Web.

The [widget tests](test/widget_test.dart) use a mock Android Wi-Fi channel and a loopback UDP receiver to exercise connection retry, settings, outgoing packets, and stop behavior.

## Code map

| File | Responsibility |
| --- | --- |
| [lib/main.dart](lib/main.dart) | Starts the UDP client, requests landscape orientation, and loads local settings before opening the controller. |
| [lib/pages/home_page.dart](lib/pages/home_page.dart) | Owns controller state, turns joystick input into a direction, applies movement limits, sends commands, and coordinates the Android Wi-Fi UI. |
| [lib/pages/settings.dart](lib/pages/settings.dart) | Edits the selected gait, walking speed and lift per gait, and Bob speed. |
| [lib/pages/admin_settings.dart](lib/pages/admin_settings.dart) and [movement_limits.dart](lib/services/movement_limits.dart) | Enable Bipod and define narrower UI ranges within the supported movement limits. The limits are stored locally and applied to commands sent by this app. |
| [lib/commands/commands.dart](lib/commands/commands.dart) | Defines command IDs, JSON fields, and default movement values. |
| [lib/services/send_to_esp.dart](lib/services/send_to_esp.dart) | Encodes commands as UTF-8 JSON, adds packet ordering fields, sends UDP, and provides a repeated Stop send. |
| [MainActivity.kt](android/app/src/main/kotlin/com/example/app/MainActivity.kt) and [RobotWifi.kt](android/app/src/main/kotlin/com/example/app/RobotWifi.kt) | Handle the `hexapod/wifi` method channel and Android network requests, permissions, and socket binding. |

The joystick sends a direction, step time, and foot lift about every 50 ms while active. Releasing it sends three Stop packets 30 ms apart. The Stop button sends one packet and suppresses further joystick updates until a new drag. Disconnecting through the Android Wi-Fi button sends a Stop burst before releasing the connection.

**Settings → Admin settings → Enable Bipod** makes Bipod available. Save the admin settings, then choose Bipod in **Settings** and save again. The selected gait resets to Tripod on launch. Speed, lift, and admin limits persist locally. Older installs with a saved `walk_step_ms` use that value for each gait until its own speed is saved.

When changing wire command IDs, fields, or movement bounds, check both the Dart definitions and the firmware's [UDP handlers](../Controls/src/hexapod_server.cpp), [timing constants](../Controls/include/movement.h), and [lift ranges](../Controls/src/movement.cpp).
