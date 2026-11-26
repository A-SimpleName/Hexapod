import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

import 'package:app/commands/commands.dart';
import 'package:app/main.dart';
import 'package:app/services/movement_limits.dart';
import 'package:app/services/send_to_esp.dart';

void main() {
  testWidgets(
    'Robot Wi-Fi can be retried and routes UDP after joining',
    (WidgetTester tester) async {
      tester.view.physicalSize = const Size(800, 400);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.resetPhysicalSize);
      addTearDown(tester.view.resetDevicePixelRatio);

      const channel = MethodChannel('hexapod/wifi');
      final messenger = tester.binding.defaultBinaryMessenger;
      final sent = <Map<String, dynamic>>[];
      var connectionAttempts = 0;
      var disconnected = false;
      messenger.setMockMethodCallHandler(channel, (call) async {
        switch (call.method) {
          case 'connect':
            expect(call.arguments, {
              'host': udpClient.espIp.address,
              'port': 4210,
            });
            if (++connectionAttempts == 1) {
              throw PlatformException(
                code: 'permission_denied',
                message: 'Wi-Fi permission is needed to connect to the robot.',
              );
            }
            return null;
          case 'send':
            sent.add(
              jsonDecode(utf8.decode(call.arguments as Uint8List))
                  as Map<String, dynamic>,
            );
            expect(disconnected, isFalse);
            return true;
          case 'disconnect':
            disconnected = true;
            return null;
          default:
            fail('Unexpected Wi-Fi method ${call.method}');
        }
      });
      addTearDown(() => messenger.setMockMethodCallHandler(channel, null));

      await tester.pumpWidget(const MyApp());
      await tester.pump(const Duration(seconds: 2));
      await tester.tap(find.text('Connect Wi-Fi'));
      await tester.pump();
      expect(
        find.text('Wi-Fi permission is needed to connect to the robot.'),
        findsOneWidget,
      );

      await tester.tap(find.text('Connect Wi-Fi'));
      await tester.pump();
      expect(find.text('Disconnect Wi-Fi'), findsOneWidget);
      await tester.tap(find.text('Stop'));
      await tester.pump();
      expect(sent.single['id'], 'stop');

      await tester.tap(find.text('Disconnect Wi-Fi'));
      await tester.pump();
      await tester.pump(const Duration(milliseconds: 30));
      await tester.pump(const Duration(milliseconds: 30));
      expect(sent.map((packet) => packet['id']), List.filled(4, 'stop'));
      expect(disconnected, isTrue);
      expect(find.text('Connect Wi-Fi'), findsOneWidget);
    },
    variant: const TargetPlatformVariant({TargetPlatform.android}),
  );

  testWidgets('Movement settings and admin limits apply to joystick commands', (
    WidgetTester tester,
  ) async {
    tester.view.physicalSize = const Size(800, 400);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    final originalPreferences = SharedPreferencesAsyncPlatform.instance;
    const oldStepMs = 520;
    SharedPreferencesAsyncPlatform.instance =
        InMemorySharedPreferencesAsync.withData({'walk_step_ms': oldStepMs});
    addTearDown(() {
      SharedPreferencesAsyncPlatform.instance = originalPreferences;
    });

    late RawDatagramSocket receiver;
    late StreamController<Map<String, dynamic>> payloads;
    late StreamSubscription<RawSocketEvent> subscription;
    late StreamIterator<Map<String, dynamic>> commands;
    final originalClient = udpClient;
    await tester.runAsync(() async {
      receiver = await RawDatagramSocket.bind(InternetAddress.loopbackIPv4, 0);
      payloads = StreamController<Map<String, dynamic>>();
      subscription = receiver.listen((event) {
        if (event == RawSocketEvent.read) {
          final datagram = receiver.receive();
          if (datagram != null) {
            payloads.add(
              jsonDecode(utf8.decode(datagram.data)) as Map<String, dynamic>,
            );
          }
        }
      });
      commands = StreamIterator(payloads.stream);
      udpClient = UdpHexapodClient('127.0.0.1', espPort: receiver.port);
      await udpClient.start();
    });
    addTearDown(
      () => tester.runAsync(() async {
        udpClient.dispose();
        udpClient = originalClient;
        await commands.cancel();
        await subscription.cancel();
        receiver.close();
        await payloads.close();
      }),
    );

    Future<Map<String, dynamic>> receiveCommand() async {
      expect(
        await commands.moveNext().timeout(const Duration(seconds: 3)),
        isTrue,
      );
      return commands.current;
    }

    final initialMovement = await loadMovementSettings();
    expect(initialMovement.tripodStepMs, oldStepMs);
    expect(initialMovement.bipodStepMs, oldStepMs);
    expect(initialMovement.tripodLiftMm, WalkGait.tripod.defaultLiftMm);
    expect(initialMovement.bipodLiftMm, WalkGait.bipod.defaultLiftMm);
    await tester.pumpWidget(MyApp(initialMovement: initialMovement));
    await tester.pump(const Duration(seconds: 2));
    await tester.tap(find.text('Bob up/down'));
    final bob = (await tester.runAsync(receiveCommand))!;
    final defaultCommand = Command.fromJson(bob) as BobUpDown;
    expect(defaultCommand.periodMs, BobUpDown.defaultPeriodMs);
    expect(bob['seq'], 1);

    await tester.tap(find.byTooltip('Settings'));
    await tester.pumpAndSettle();
    String bobTime(int ms) => 'Cycle time: ${(ms / 1000).toStringAsFixed(1)} s';
    expect(find.text(bobTime(BobUpDown.defaultPeriodMs)), findsOneWidget);
    expect(find.text('Bipod'), findsNothing);
    expect(find.byKey(const ValueKey('bipod-speed')), findsNothing);
    await tester.tap(find.text('Admin settings'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Enable Bipod'));
    await tester.tap(find.text('Save'));
    await tester.pumpAndSettle();
    expect((await MovementLimits.load()).bipodEnabled, isTrue);
    await tester.tap(find.text('Bipod'));
    await tester.pumpAndSettle();
    final bobControl = find.byKey(const ValueKey('bob-speed'));
    await tester.ensureVisible(bobControl);
    final slider = tester.getRect(bobControl);
    await tester.tapAt(Offset(slider.right - 8, slider.center.dy));
    await tester.pumpAndSettle();
    expect(find.text(bobTime(BobUpDown.minPeriodMs)), findsOneWidget);
    expect(find.text('Step time: $oldStepMs ms'), findsNWidgets(2));
    final tripodSlider = tester.getRect(
      find.byKey(const ValueKey('tripod-speed')),
    );
    await tester.tapAt(Offset(tripodSlider.left + 8, tripodSlider.center.dy));
    await tester.pumpAndSettle();
    expect(find.text('Step time: ${Move.maxStepMs} ms'), findsOneWidget);
    expect(find.text('Step time: $oldStepMs ms'), findsOneWidget);
    final bipodSlider = tester.getRect(
      find.byKey(const ValueKey('bipod-speed')),
    );
    await tester.tapAt(Offset(bipodSlider.right - 8, bipodSlider.center.dy));
    await tester.pumpAndSettle();
    expect(find.text('Step time: ${Move.minStepMs} ms'), findsOneWidget);
    final tripodHeight = find.byKey(const ValueKey('tripod-height'));
    await tester.ensureVisible(tripodHeight);
    final tripodHeightSlider = tester.getRect(tripodHeight);
    await tester.tapAt(
      Offset(tripodHeightSlider.right - 8, tripodHeightSlider.center.dy),
    );
    await tester.pumpAndSettle();
    expect(find.text('Lift: ${WalkGait.tripod.maxLiftMm} mm'), findsOneWidget);
    expect(
      find.text('Lift: ${WalkGait.bipod.defaultLiftMm} mm'),
      findsOneWidget,
    );
    final bipodHeight = find.byKey(const ValueKey('bipod-height'));
    await tester.ensureVisible(bipodHeight);
    final bipodHeightSlider = tester.getRect(bipodHeight);
    await tester.tapAt(
      Offset(bipodHeightSlider.left + 8, bipodHeightSlider.center.dy),
    );
    await tester.pumpAndSettle();
    expect(find.text('Lift: ${WalkGait.bipod.minLiftMm} mm'), findsOneWidget);
    await tester.ensureVisible(find.text('Save'));
    await tester.tap(find.text('Save'));
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 400));
    expect(
      await SharedPreferencesAsync().getInt(bobPeriodPreferenceKey),
      BobUpDown.minPeriodMs,
    );
    final reloadedMovement = await loadMovementSettings();
    expect(reloadedMovement.tripodStepMs, Move.maxStepMs);
    expect(reloadedMovement.bipodStepMs, Move.minStepMs);
    expect(reloadedMovement.tripodLiftMm, WalkGait.tripod.maxLiftMm);
    expect(reloadedMovement.bipodLiftMm, WalkGait.bipod.minLiftMm);

    await tester.tap(find.text('Bob up/down'));
    final adjustedBob = (await tester.runAsync(receiveCommand))!;
    expect(adjustedBob['id'], 'bob_up_down');
    expect(adjustedBob['period_ms'], BobUpDown.minPeriodMs);
    expect(adjustedBob['seq'], 2);

    await tester.tap(find.text('Stop'));
    final stop = (await tester.runAsync(receiveCommand))!;
    expect(stop['id'], 'stop');
    expect(stop['seq'], 3);
    expect(stop['session'], bob['session']);

    Future<void> verifyWalk(
      String id,
      WalkGait gait,
      int stepMs,
      int liftMm,
    ) async {
      final drag = await tester.startGesture(
        tester.getCenter(find.byType(Joystick)),
      );
      await drag.moveBy(const Offset(40, 0));
      await tester.pump(const Duration(milliseconds: 55));
      final walk = (await tester.runAsync(receiveCommand))!;
      expect(walk['id'], id);
      final walkCommand = Command.fromJson(walk) as Move;
      expect(walkCommand.stepMs, stepMs);
      expect(walkCommand.liftMm, liftMm);
      expect(walkCommand.gait, gait);
      await tester.tap(find.text('Stop'), pointer: 2);
      await tester.pump(const Duration(milliseconds: 55));
      expect((await tester.runAsync(receiveCommand))!['id'], 'stop');
      await drag.up();
      await tester.pump();
      for (var i = 0; i < 3; i++) {
        if (i > 0) await tester.pump(const Duration(milliseconds: 30));
        final release = (await tester.runAsync(receiveCommand))!;
        expect(release['id'], 'stop');
      }
    }

    await verifyWalk(
      'move_bipod',
      WalkGait.bipod,
      Move.minStepMs,
      WalkGait.bipod.minLiftMm,
    );
    await tester.tap(find.byTooltip('Settings'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Admin settings'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Enable Bipod'));

    Future<void> setBound(String key, int value) async {
      final field = find.byKey(ValueKey(key));
      await tester.ensureVisible(field);
      await tester.enterText(field, '$value');
    }

    await setBound('tripodSpeed-min', 500);
    await setBound('tripodSpeed-max', 801);
    await tester.tap(find.text('Save'));
    await tester.pumpAndSettle();
    expect(
      find.text('Use ${Move.minStepMs}–${Move.maxStepMs}'),
      findsOneWidget,
    );
    await setBound('tripodSpeed-max', 499);
    await tester.tap(find.text('Save'));
    await tester.pumpAndSettle();
    expect(find.text('Below minimum'), findsOneWidget);
    expect((await MovementLimits.load()).bipodEnabled, isTrue);
    await setBound('tripodSpeed-max', 620);
    await setBound('tripodHeight-max', 18);
    await setBound('bobSpeed-min', 1225);
    await setBound('bobSpeed-max', 1225);
    await tester.tap(find.text('Save'));
    await tester.pump();
    for (var i = 0; i < 3; i++) {
      if (i > 0) await tester.pump(const Duration(milliseconds: 30));
      expect((await tester.runAsync(receiveCommand))!['id'], 'stop');
    }
    await tester.pumpAndSettle();
    expect(find.text('Bipod'), findsNothing);
    expect(find.byKey(const ValueKey('bipod-height')), findsNothing);
    expect(find.text('Step time: 620 ms'), findsOneWidget);
    await tester.ensureVisible(find.byKey(const ValueKey('bob-speed')));
    expect(find.text('Cycle time: 1.225 s'), findsOneWidget);
    expect(
      tester.widget<Slider>(find.byKey(const ValueKey('bob-speed'))).onChanged,
      isNull,
    );
    final limits = await MovementLimits.load();
    expect(limits.bipodEnabled, isFalse);
    expect(limits.rangeFor(MotionControl.tripodSpeed), (min: 500, max: 620));
    expect((await loadMovementSettings()).tripodStepMs, Move.maxStepMs);
    await tester.tap(find.byTooltip('Close Settings'));
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 400));
    await tester.tap(find.text('Bob up/down'));
    expect((await tester.runAsync(receiveCommand))!['period_ms'], 1225);
    await verifyWalk('move', WalkGait.tripod, 620, 18);
    expect(tester.takeException(), isNull);
  });
}
