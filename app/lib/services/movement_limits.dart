import 'dart:convert';

import 'package:app/commands/commands.dart';
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

typedef MotionRange = ({int min, int max});

enum MotionControl {
  bobSpeed('Bob cycle time', 'ms'),
  tripodSpeed('Tripod step time', 'ms'),
  tripodHeight('Tripod step height', 'mm'),
  bipodSpeed('Bipod step time', 'ms'),
  bipodHeight('Bipod step height', 'mm');

  const MotionControl(this.label, this.unit);

  final String label;
  final String unit;

  MotionRange get firmwareRange => switch (this) {
    bobSpeed => (min: BobUpDown.minPeriodMs, max: BobUpDown.maxPeriodMs),
    tripodSpeed || bipodSpeed => (min: Move.minStepMs, max: Move.maxStepMs),
    tripodHeight => (
      min: WalkGait.tripod.minLiftMm,
      max: WalkGait.tripod.maxLiftMm,
    ),
    bipodHeight => (
      min: WalkGait.bipod.minLiftMm,
      max: WalkGait.bipod.maxLiftMm,
    ),
  };

  bool accepts(int min, int max) {
    final allowed = firmwareRange;
    return min >= allowed.min && max <= allowed.max && min <= max;
  }
}

extension WalkGaitMotionControls on WalkGait {
  MotionControl get stepTimeControl => this == WalkGait.bipod
      ? MotionControl.bipodSpeed
      : MotionControl.tripodSpeed;

  MotionControl get stepHeightControl => this == WalkGait.bipod
      ? MotionControl.bipodHeight
      : MotionControl.tripodHeight;
}

class MovementLimits {
  static const _preferenceKey = 'movement_limits';

  final bool bipodEnabled;
  final Map<MotionControl, MotionRange> _ranges;

  const MovementLimits.defaults() : bipodEnabled = false, _ranges = const {};

  MovementLimits({
    required this.bipodEnabled,
    required Map<MotionControl, MotionRange> ranges,
  }) : _ranges = Map.unmodifiable(ranges);

  MotionRange rangeFor(MotionControl control) =>
      _ranges[control] ?? control.firmwareRange;

  int limit(MotionControl control, int value) {
    final range = rangeFor(control);
    return value.clamp(range.min, range.max);
  }

  static Future<MovementLimits> load() async {
    try {
      final saved = await SharedPreferencesAsync().getString(_preferenceKey);
      if (saved == null) return const MovementLimits.defaults();
      if (jsonDecode(saved) case final Map<String, dynamic> data) {
        final ranges = <MotionControl, MotionRange>{};
        for (final control in MotionControl.values) {
          if (data[control.name] case [
            final int min,
            final int max,
          ] when control.accepts(min, max)) {
            ranges[control] = (min: min, max: max);
          }
        }
        return MovementLimits(
          bipodEnabled: data['bipodEnabled'] == true,
          ranges: ranges,
        );
      }
    } catch (error) {
      debugPrint('Could not load movement limits: $error');
    }
    return const MovementLimits.defaults();
  }

  Future<void> save() => SharedPreferencesAsync().setString(
    _preferenceKey,
    jsonEncode({
      'bipodEnabled': bipodEnabled,
      for (final entry in _ranges.entries)
        entry.key.name: [entry.value.min, entry.value.max],
    }),
  );
}
