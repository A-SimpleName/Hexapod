import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:app/commands/commands.dart';
import 'package:app/main.dart';
import 'package:app/services/send_to_esp.dart';

void main() {
  testWidgets('Bob button sends a command and Stop remains available', (
    WidgetTester tester,
  ) async {
    tester.view.physicalSize = const Size(800, 400);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);

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

    await tester.pumpWidget(const MyApp());
    await tester.pump(const Duration(seconds: 2));
    await tester.tap(find.text('Bob up/down'));
    final bob = (await tester.runAsync(receiveCommand))!;
    expect(Command.fromJson(bob), isA<BobUpDown>());
    expect(bob['id'], 'bob_up_down');
    expect(bob['seq'], 1);

    await tester.tap(find.text('Stop'));
    final stop = (await tester.runAsync(receiveCommand))!;
    expect(stop['id'], 'stop');
    expect(stop['seq'], 2);
    expect(stop['session'], bob['session']);
    expect(tester.takeException(), isNull);
  });
}
