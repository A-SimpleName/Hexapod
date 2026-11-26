import 'package:app/commands/commands.dart';
import 'package:flutter/material.dart';

typedef MovementSettings = ({
  int bobPeriodMs,
  int tripodStepMs,
  int bipodStepMs,
  int tripodLiftMm,
  int bipodLiftMm,
  WalkGait gait,
});

class SettingsPage extends StatefulWidget {
  final MovementSettings initialSettings;

  const SettingsPage({super.key, required this.initialSettings});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late int _bobPeriodMs = widget.initialSettings.bobPeriodMs;
  late final _stepTimes = {
    WalkGait.tripod: widget.initialSettings.tripodStepMs,
    WalkGait.bipod: widget.initialSettings.bipodStepMs,
  };
  late final _liftHeights = {
    WalkGait.tripod: widget.initialSettings.tripodLiftMm,
    WalkGait.bipod: widget.initialSettings.bipodLiftMm,
  };
  late WalkGait _gait = widget.initialSettings.gait;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
        actions: [
          IconButton(
            icon: const Icon(Icons.close),
            tooltip: 'Close Settings',
            onPressed: () => Navigator.pop(context),
          ),
        ],
      ),
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 720),
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(20),
              child: LayoutBuilder(
                builder: (context, constraints) {
                  const gap = 24.0;
                  final width = constraints.maxWidth;
                  final columns = width >= 600 ? 3 : (width >= 400 ? 2 : 1);
                  final controlWidth = ((width - gap * (columns - 1)) / columns)
                      .floorToDouble();
                  return Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Text(
                        'Walking gait',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 8),
                      Align(
                        alignment: Alignment.centerLeft,
                        child: SegmentedButton<WalkGait>(
                          segments: const [
                            ButtonSegment(
                              value: WalkGait.tripod,
                              label: Text('Tripod'),
                            ),
                            ButtonSegment(
                              value: WalkGait.bipod,
                              label: Text('Bipod'),
                            ),
                          ],
                          selected: {_gait},
                          onSelectionChanged: (selected) =>
                              setState(() => _gait = selected.single),
                        ),
                      ),
                      if (_gait == WalkGait.bipod)
                        const Padding(
                          padding: EdgeInsets.only(top: 8),
                          child: Text(
                            'Experimental. Two feet support the robot during each step.',
                          ),
                        ),
                      const SizedBox(height: 20),
                      Wrap(
                        spacing: gap,
                        runSpacing: 24,
                        children: [
                          SizedBox(
                            width: controlWidth,
                            child: _MotionSlider(
                              sliderKey: const ValueKey('bob-speed'),
                              title: 'Bob speed',
                              valueLabel: 'Cycle time',
                              value: _bobPeriodMs,
                              min: BobUpDown.minPeriodMs,
                              max: BobUpDown.maxPeriodMs,
                              increment: 200,
                              format: (ms) =>
                                  '${(ms / 1000).toStringAsFixed(1)} s',
                              fasterOnRight: true,
                              onChanged: (ms) =>
                                  setState(() => _bobPeriodMs = ms),
                            ),
                          ),
                          for (final gait in WalkGait.values)
                            SizedBox(
                              width: controlWidth,
                              child: Column(
                                children: [
                                  _MotionSlider(
                                    sliderKey: ValueKey('${gait.name}-speed'),
                                    title: '${gait.label} speed',
                                    valueLabel: 'Step time',
                                    value: _stepTimes[gait]!,
                                    min: Move.minStepMs,
                                    max: Move.maxStepMs,
                                    increment: 20,
                                    format: (ms) => '$ms ms',
                                    fasterOnRight: true,
                                    onChanged: (ms) =>
                                        setState(() => _stepTimes[gait] = ms),
                                  ),
                                  const SizedBox(height: 16),
                                  _MotionSlider(
                                    sliderKey: ValueKey('${gait.name}-height'),
                                    title: '${gait.label} step height',
                                    valueLabel: 'Lift',
                                    value: _liftHeights[gait]!,
                                    min: gait.minLiftMm,
                                    max: gait.maxLiftMm,
                                    increment: 2,
                                    format: (mm) => '$mm mm',
                                    onChanged: (mm) =>
                                        setState(() => _liftHeights[gait] = mm),
                                  ),
                                ],
                              ),
                            ),
                        ],
                      ),
                      const SizedBox(height: 20),
                      Align(
                        alignment: Alignment.centerRight,
                        child: FilledButton(
                          onPressed: () =>
                              Navigator.pop<MovementSettings>(context, (
                                bobPeriodMs: _bobPeriodMs,
                                tripodStepMs: _stepTimes[WalkGait.tripod]!,
                                bipodStepMs: _stepTimes[WalkGait.bipod]!,
                                tripodLiftMm: _liftHeights[WalkGait.tripod]!,
                                bipodLiftMm: _liftHeights[WalkGait.bipod]!,
                                gait: _gait,
                              )),
                          child: const Text('Save'),
                        ),
                      ),
                    ],
                  );
                },
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _MotionSlider extends StatelessWidget {
  final Key sliderKey;
  final String title;
  final String valueLabel;
  final int value;
  final int min;
  final int max;
  final int increment;
  final String Function(int) format;
  final bool fasterOnRight;
  final ValueChanged<int> onChanged;

  const _MotionSlider({
    required this.sliderKey,
    required this.title,
    required this.valueLabel,
    required this.value,
    required this.min,
    required this.max,
    required this.increment,
    required this.format,
    this.fasterOnRight = false,
    required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    final formatted = format(value);
    int fromSlider(double position) =>
        fasterOnRight ? max - position.round() : min + position.round();
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Text(title, style: Theme.of(context).textTheme.titleLarge),
        Text('$valueLabel: $formatted'),
        Semantics(
          label: title,
          child: Slider(
            key: sliderKey,
            value: (fasterOnRight ? max - value : value - min).toDouble(),
            max: (max - min).toDouble(),
            divisions: (max - min) ~/ increment,
            label: formatted,
            semanticFormatterCallback: (value) => format(fromSlider(value)),
            onChanged: (value) => onChanged(fromSlider(value)),
          ),
        ),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: fasterOnRight
              ? const [Text('Slower'), Text('Faster')]
              : const [Text('Lower'), Text('Higher')],
        ),
      ],
    );
  }
}
