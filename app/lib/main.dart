import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:app/commands/commands.dart';
import 'package:app/pages/home_page.dart';
import 'package:app/pages/settings.dart';
import 'package:app/services/movement_limits.dart';
import 'package:app/services/send_to_esp.dart';
import 'package:shared_preferences/shared_preferences.dart';

UdpHexapodClient udpClient = UdpHexapodClient('192.168.4.1');
const bobPeriodPreferenceKey = 'bob_period_ms';
const rotateStepPreferenceKey = 'rotate_step_ms';
const tripodStepPreferenceKey = 'tripod_step_ms';
const bipodStepPreferenceKey = 'bipod_step_ms';
const tripodLiftPreferenceKey = 'tripod_lift_mm';
const bipodLiftPreferenceKey = 'bipod_lift_mm';
const _legacyWalkStepPreferenceKey = 'walk_step_ms';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]);

  try {
    await udpClient.start();
  } catch (e) {
    debugPrint('[UDP] FEHLER beim Initialisieren des UDP-Clients: $e');
  }

  runApp(
    MyApp(
      initialMovement: await loadMovementSettings(),
      initialLimits: await MovementLimits.load(),
    ),
  );
}

Future<MovementSettings> loadMovementSettings() async {
  final bobPeriodMs = await _loadSetting(
    bobPeriodPreferenceKey,
    BobUpDown.defaultPeriodMs,
    BobUpDown.minPeriodMs,
    BobUpDown.maxPeriodMs,
  );
  // Seed each gait from the old shared value until that gait is edited.
  final legacyStepMs = await _loadSetting(
    _legacyWalkStepPreferenceKey,
    Move.defaultStepMs,
    Move.minStepMs,
    Move.maxStepMs,
  );
  return (
    bobPeriodMs: bobPeriodMs,
    rotateStepMs: await _loadSetting(
      rotateStepPreferenceKey,
      Rotate.defaultStepMs,
      Rotate.minStepMs,
      Rotate.maxStepMs,
    ),
    tripodStepMs: await _loadSetting(
      tripodStepPreferenceKey,
      legacyStepMs,
      Move.minStepMs,
      Move.maxStepMs,
    ),
    bipodStepMs: await _loadSetting(
      bipodStepPreferenceKey,
      legacyStepMs,
      Move.minStepMs,
      Move.maxStepMs,
    ),
    tripodLiftMm: await _loadSetting(
      tripodLiftPreferenceKey,
      WalkGait.tripod.defaultLiftMm,
      WalkGait.tripod.minLiftMm,
      WalkGait.tripod.maxLiftMm,
    ),
    bipodLiftMm: await _loadSetting(
      bipodLiftPreferenceKey,
      WalkGait.bipod.defaultLiftMm,
      WalkGait.bipod.minLiftMm,
      WalkGait.bipod.maxLiftMm,
    ),
    gait: WalkGait.tripod,
  );
}

Future<int> _loadSetting(String key, int defaultValue, int min, int max) async {
  try {
    final saved = await SharedPreferencesAsync().getInt(key);
    if (saved != null && saved >= min && saved <= max) {
      return saved;
    }
  } catch (e) {
    debugPrint('Could not load $key: $e');
  }
  return defaultValue;
}

class MyApp extends StatelessWidget {
  final MovementSettings? initialMovement;
  final MovementLimits initialLimits;

  const MyApp({
    super.key,
    this.initialMovement,
    this.initialLimits = const MovementLimits.defaults(),
  });

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        // This is the theme of your application.
        //
        // TRY THIS: Try running your application with "flutter run". You'll see
        // the application has a purple toolbar. Then, without quitting the app,
        // try changing the seedColor in the colorScheme below to Colors.green
        // and then invoke "hot reload" (save your changes or press the "hot
        // reload" button in a Flutter-supported IDE, or press "r" if you used
        // the command line to start the app).
        //
        // Notice that the counter didn't reset back to zero; the application
        // state is not lost during the reload. To reset the state, use hot
        // restart instead.
        //
        // This works for code too, not just values: Most code changes can be
        // tested with just a hot reload.
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
      ),
      initialRoute: 'HomePage',
      routes: {
        'HomePage': (context) => HomePage(
          title: 'Hexapod Controls',
          initialLimits: initialLimits,
          initialMovement:
              initialMovement ??
              (
                bobPeriodMs: BobUpDown.defaultPeriodMs,
                rotateStepMs: Rotate.defaultStepMs,
                tripodStepMs: Move.defaultStepMs,
                bipodStepMs: Move.defaultStepMs,
                tripodLiftMm: WalkGait.tripod.defaultLiftMm,
                bipodLiftMm: WalkGait.bipod.defaultLiftMm,
                gait: WalkGait.tripod,
              ),
        ),
      },
    );
  }
}
