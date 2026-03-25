import 'package:flutter/material.dart';

class SettingsPage extends StatelessWidget {
  final String title;

  const SettingsPage({super.key, required this.title});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(title),
        actions: <Widget>[
          IconButton(
            icon: const Icon(Icons.close),
            tooltip: 'Close Settings',
            onPressed: () {
              Navigator.pop(context);
            },
          ),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            children: [...List.generate(10, (index) => Container(
              margin: const EdgeInsets.symmetric(vertical: 8.0),
              padding: const EdgeInsets.all(8.0),
              decoration: BoxDecoration(
                border: Border.all(color: const Color.fromARGB(255, 57, 204, 220)),
                color: const Color.fromARGB(255, 210, 210, 210),
              ),
              width: double.infinity,
              child: Text('Setting Option ${index + 1}'),
            ))],
          ),
        ),
      ),
    );
  }
}
