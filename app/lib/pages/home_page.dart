import 'dart:math';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:app/pages/settings.dart';
import 'package:app/services/movement_limits.dart';
import 'package:app/main.dart';
import 'package:app/commands/commands.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:shared_preferences/shared_preferences.dart';

class HomePage extends StatefulWidget {
  final String title;
  final MovementSettings initialMovement;
  final MovementLimits initialLimits;

  const HomePage({
    super.key,
    required this.title,
    required this.initialMovement,
    required this.initialLimits,
  });

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  static const _wifiChannel = MethodChannel('hexapod/wifi');
  final _showWifiConnect =
      !kIsWeb && defaultTargetPlatform == TargetPlatform.android;
  bool _wifiConnected = false;
  bool _wifiBusy = false;
  bool _joystickStopped = false;

  late MovementSettings _movement = widget.initialMovement;
  late MovementLimits _limits = widget.initialLimits;

  @override
  void initState() {
    super.initState();
    if (_showWifiConnect) {
      _wifiChannel.setMethodCallHandler((call) async {
        if (call.method == 'disconnected' && mounted) {
          udpClient.robotWifiSender = null;
          setState(() => _wifiConnected = false);
          _showWifiMessage('Robot Wi-Fi disconnected.');
        }
      });
    }
  }

  @override
  void dispose() {
    if (_showWifiConnect) {
      _wifiChannel.setMethodCallHandler(null);
      udpClient.robotWifiSender = null;
    }
    super.dispose();
  }

  void _showWifiMessage(String message) {
    final messenger = ScaffoldMessenger.of(context);
    messenger.hideCurrentSnackBar();
    messenger.showSnackBar(SnackBar(content: Text(message)));
  }

  Future<void> _toggleRobotWifi() async {
    setState(() => _wifiBusy = true);
    try {
      if (_wifiConnected) {
        _joystickStopped = true;
        await udpClient.sendStopBurst();
        await _wifiChannel.invokeMethod<void>('disconnect');
        if (!mounted) return;
        udpClient.robotWifiSender = null;
        setState(() => _wifiConnected = false);
      } else {
        await _wifiChannel.invokeMethod<void>('connect', {
          'host': udpClient.espIp.address,
          'port': udpClient.espPort,
        });
        if (!mounted) return;
        udpClient.robotWifiSender = (packet) async =>
            await _wifiChannel.invokeMethod<bool>('send', packet) ?? false;
        setState(() => _wifiConnected = true);
        _showWifiMessage('Connected to Hexapod-ESP32.');
      }
    } on PlatformException catch (error) {
      if (mounted) {
        _showWifiMessage(error.message ?? 'Could not connect to the robot.');
      }
    } on MissingPluginException {
      if (mounted) {
        _showWifiMessage('In-app Wi-Fi is unavailable on this device.');
      }
    } finally {
      if (mounted) setState(() => _wifiBusy = false);
    }
  }

  WalkGait get _activeGait =>
      _limits.bipodEnabled ? _movement.gait : WalkGait.tripod;

  Future<void> _saveLimits(MovementLimits limits) async {
    await limits.save();
    if (!mounted) return;
    final leavingBipod = _activeGait == WalkGait.bipod && !limits.bipodEnabled;
    setState(() {
      _limits = limits;
      if (leavingBipod) {
        _joystickStopped = true;
        _movement = (
          bobPeriodMs: _movement.bobPeriodMs,
          tripodStepMs: _movement.tripodStepMs,
          bipodStepMs: _movement.bipodStepMs,
          tripodLiftMm: _movement.tripodLiftMm,
          bipodLiftMm: _movement.bipodLiftMm,
          gait: WalkGait.tripod,
        );
      }
    });
    if (leavingBipod) await udpClient.sendStopBurst();
  }

  Future<void> _openSettings() async {
    final settings = await Navigator.push<MovementSettings>(
      context,
      MaterialPageRoute(
        builder: (_) => SettingsPage(
          initialSettings: _movement,
          initialLimits: _limits,
          onSaveLimits: _saveLimits,
        ),
      ),
    );
    if (!mounted || settings == null || settings == _movement) {
      return;
    }
    final previous = _movement;
    setState(() => _movement = settings);
    final changed = {
      if (settings.bobPeriodMs != previous.bobPeriodMs)
        bobPeriodPreferenceKey: settings.bobPeriodMs,
      if (settings.tripodStepMs != previous.tripodStepMs)
        tripodStepPreferenceKey: settings.tripodStepMs,
      if (settings.bipodStepMs != previous.bipodStepMs)
        bipodStepPreferenceKey: settings.bipodStepMs,
      if (settings.tripodLiftMm != previous.tripodLiftMm)
        tripodLiftPreferenceKey: settings.tripodLiftMm,
      if (settings.bipodLiftMm != previous.bipodLiftMm)
        bipodLiftPreferenceKey: settings.bipodLiftMm,
    };
    if (changed.isEmpty) return;
    try {
      final preferences = SharedPreferencesAsync();
      await Future.wait(
        changed.entries.map(
          (entry) => preferences.setInt(entry.key, entry.value),
        ),
      );
    } catch (error) {
      debugPrint('Could not save movement settings: $error');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Could not save movement settings for next launch.'),
          ),
        );
      }
    }
  }

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
        title: Text(widget.title),
        actions: <Widget>[
          if (_showWifiConnect)
            TextButton.icon(
              onPressed: _wifiBusy ? null : _toggleRobotWifi,
              icon: Icon(_wifiConnected ? Icons.wifi : Icons.wifi_off),
              label: Text(
                _wifiBusy
                    ? (_wifiConnected ? 'Disconnecting…' : 'Connecting…')
                    : (_wifiConnected ? 'Disconnect Wi-Fi' : 'Connect Wi-Fi'),
              ),
            ),
          IconButton(
            icon: const Icon(Icons.settings),
            tooltip: 'Settings',
            onPressed: _openSettings,
          ),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            children: [
              SizedBox(
                width: 180,
                height: double.infinity,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(
                      child: ElevatedButton(
                        onPressed: () async {
                          _joystickStopped = true;
                          await _sendAndNotify(context, StopCommand());
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.red,
                        ),
                        child: const Text('Stop'),
                      ),
                    ),
                    const SizedBox(height: 12),
                    SizedBox(
                      height: 56,
                      child: ElevatedButton.icon(
                        onPressed: () async => _sendAndNotify(
                          context,
                          BobUpDown(
                            periodMs: _limits.limit(
                              MotionControl.bobSpeed,
                              _movement.bobPeriodMs,
                            ),
                          ),
                        ),
                        icon: const Icon(Icons.height),
                        label: const Text('Bob up/down'),
                      ),
                    ),
                  ],
                ),
              ),
              const Spacer(),
              SizedBox(
                width: 260,
                height: 260,
                child: Joystick(
                  mode: JoystickMode.all,
                  onStickDragStart: () => _joystickStopped = false,
                  listener: (StickDragDetails details) {
                    if (_joystickStopped) return;
                    final gait = _activeGait;
                    if (gait == WalkGait.bipod &&
                        details.x == 0 &&
                        details.y == 0) {
                      return;
                    }
                    final theta =
                        360.0 - atan2(details.x, details.y) * 180 / pi;
                    udpClient.sendCommand(
                      Move(
                        theta: theta,
                        stepMs: gait == WalkGait.bipod
                            ? _limits.limit(
                                MotionControl.bipodSpeed,
                                _movement.bipodStepMs,
                              )
                            : _limits.limit(
                                MotionControl.tripodSpeed,
                                _movement.tripodStepMs,
                              ),
                        liftMm: gait == WalkGait.bipod
                            ? _limits.limit(
                                MotionControl.bipodHeight,
                                _movement.bipodLiftMm,
                              )
                            : _limits.limit(
                                MotionControl.tripodHeight,
                                _movement.tripodLiftMm,
                              ),
                        gait: gait,
                      ),
                    );
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
