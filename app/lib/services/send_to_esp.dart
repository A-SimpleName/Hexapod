import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:app/commands/commands.dart';

class UdpHexapodClient {
  final InternetAddress espIp;
  final int espPort;

  RawDatagramSocket? _socket;
  Future<void>? _startFuture;

  UdpHexapodClient(String ip, {this.espPort = 4210})
      : espIp = InternetAddress(ip);

  Future<void> start() {
    _startFuture ??= _initSocket();
    return _startFuture!;
  }

  Future<void> _initSocket() async {
    _socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
    debugPrint('[UDP] Socket gebunden auf ${_socket!.address.address}:${_socket!.port}');
    // optional: _socket!.broadcastEnabled = true;
  }

  Future<bool> sendCommand(Command cmd) async {
    await start();

    final socket = _socket;
    if (socket == null) {
      debugPrint('[UDP] FEHLER: Socket ist null, Befehl ${cmd.id} wurde nicht gesendet.');
      return false;
    }

    final data = utf8.encode(jsonEncode(cmd.toJson()));
    try {
      final sentBytes = socket.send(data, espIp, espPort);
      debugPrint('[UDP] Sende ${cmd.id} an ${espIp.address}:$espPort (Bytes: $sentBytes, Payload: ${utf8.decode(data)})');
      if (sentBytes == 0) {
        debugPrint('[UDP] WARNUNG: 0 Bytes gesendet. Ziel nicht erreichbar oder Socket-Problem moeglich.');
        return false;
      }
      return true;
    } on SocketException catch (e) {
      debugPrint('[UDP] SocketException beim Senden von ${cmd.id}: $e');
      return false;
    } catch (e) {
      debugPrint('[UDP] Unerwarteter Fehler beim Senden von ${cmd.id}: $e');
      return false;
    }
  }

  void dispose() {
    _socket?.close();
    _socket = null;
    _startFuture = null;
  }
}
