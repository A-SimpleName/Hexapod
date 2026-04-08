import 'dart:math';

import 'package:flutter/material.dart';
import 'package:app/pages/settings.dart';
import 'package:app/main.dart';
import 'package:app/commands/commands.dart';
import 'package:flutter_joystick/flutter_joystick.dart';

class HomePage extends StatelessWidget {
  final String title;

  const HomePage({super.key, required this.title});

  Future<void> _sendAndNotify(BuildContext context, Command command) async {
    final ok = await udpClient.sendCommand(command);
    var message = ok
        ? 'Befehl ${command.id} gesendet'
        : 'Senden von ${command.id} fehlgeschlagen';

    if (command is Move && command.theta != null) {
      message = '$message (theta: ${command.theta!.toStringAsFixed(1)}°)';
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(title),
        actions: <Widget>[
          IconButton(
            icon: const Icon(Icons.settings),
            tooltip: 'Settings',
            onPressed: () {
              Navigator.push(context, MaterialPageRoute(builder: (context) => SettingsPage(title: 'Settings')));
            },
          ),
        ]  
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              SizedBox(
                width: 180,
                height: double.infinity,
                child: ElevatedButton(
                  onPressed: () async => _sendAndNotify(context, StopCommand()),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                  child: const Text('Stop'),
                ),
              ),
              const Spacer(),
              SizedBox(
                width: 260,
                height: 260,
                child: Joystick(
                  mode: JoystickMode.all,
                  listener: (StickDragDetails details) {
                    final theta = 360.0 - atan2(details.x, details.y) * 180 / pi;
                    udpClient.sendCommand(Move(theta: theta));
                  },
                  period: const Duration(milliseconds: 50),
                  onStickDragEnd: () async {
                    await udpClient.sendStopBurst();
                  },
                  stick: JoystickStick(
                    decoration: JoystickStickDecoration(
                      color: Colors.blueAccent,
                      shadowColor: Colors.black.withAlpha(100),
                    ),
                  ),
                  base: JoystickBase(
                    decoration: JoystickBaseDecoration(
                      color: Colors.black,
                      boxShadowColor: Colors.black.withAlpha(100),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
