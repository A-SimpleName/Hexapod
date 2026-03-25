import 'package:flutter/material.dart';
import 'package:app/pages/settings.dart';
import 'package:app/main.dart';
import 'package:app/commands/commands.dart';

class HomePage extends StatelessWidget {
  final String title;

  const HomePage({super.key, required this.title});

  Future<void> _sendAndNotify(BuildContext context, Command command) async {
    final ok = await udpClient.sendCommand(command);
    final message = ok
        ? 'Befehl ${command.id} gesendet'
        : 'Senden von ${command.id} fehlgeschlagen';

    if (context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message)));
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
      body: GridView.count(
        crossAxisCount: 3,
        children: [
          ElevatedButton(
            onPressed: () async => _sendAndNotify(context, WaveLegs()),
            child: const Text('Wave Legs'),
          ),
          ElevatedButton(
            onPressed: () async => _sendAndNotify(context, StopCommand()),
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Stop'),
          ),
          ElevatedButton(
            onPressed: () async => _sendAndNotify(context, CircleJerk()),
            child: const Text('Circle Jerk'),
          ),
        ]
      ),
    );
  }
}
