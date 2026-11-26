abstract class Command {
  final String id;

  Command({required this.id});

  Map<String, dynamic> toJson() => {'id': id};

  Map<String, dynamic> get parameters => const {};

  static Command fromJson(Map<String, dynamic> json) {
    final id = (json['id'] ?? '').toString();

    switch (id) {
      case 'wave_legs':
        return WaveLegs.fromJson(json);
      case 'circle_jerk':
        return CircleJerk.fromJson(json);
      case 'bob_up_down':
        return BobUpDown.fromJson(json);
      case 'move':
      case 'move_bipod':
        return Move.fromJson(json);
      case 'stop':
        return StopCommand();
      default:
        throw FormatException('Unknown command id: $id');
    }
  }

  static double? readDouble(dynamic value) {
    if (value == null) {
      return null;
    }
    if (value is num) {
      return value.toDouble();
    }
    if (value is String) {
      return double.tryParse(value);
    }
    return null;
  }
}

class WaveLegs extends Command {
  final List<double>? legOffsets;
  final double? speed;

  WaveLegs({this.legOffsets, this.speed}) : super(id: 'wave_legs');

  factory WaveLegs.fromJson(Map<String, dynamic> json) {
    final offsetsRaw = json['legOffsets'];
    final offsets = offsetsRaw is List
        ? offsetsRaw
              .map((value) => Command.readDouble(value))
              .whereType<double>()
              .toList()
        : null;

    return WaveLegs(
      legOffsets: offsets,
      speed: Command.readDouble(json['speed']),
    );
  }

  @override
  Map<String, dynamic> toJson() => {
    'id': id,
    if (legOffsets != null) 'legOffsets': legOffsets,
    if (speed != null) 'speed': speed,
  };

  @override
  Map<String, dynamic> get parameters => {
    if (legOffsets != null) 'legOffsets': legOffsets,
    if (speed != null) 'speed': speed,
  };
}

class CircleJerk extends Command {
  final double? speed;

  CircleJerk({this.speed}) : super(id: 'circle_jerk');

  factory CircleJerk.fromJson(Map<String, dynamic> json) {
    return CircleJerk(speed: Command.readDouble(json['speed']));
  }

  @override
  Map<String, dynamic> toJson() => {
    'id': id,
    if (speed != null) 'speed': speed,
  };

  @override
  Map<String, dynamic> get parameters => {if (speed != null) 'speed': speed};
}

class BobUpDown extends Command {
  static const defaultPeriodMs = 2400;
  static const minPeriodMs = 1600;
  static const maxPeriodMs = 4800;

  final int? periodMs;

  BobUpDown({this.periodMs}) : super(id: 'bob_up_down');

  factory BobUpDown.fromJson(Map<String, dynamic> json) {
    final period = Command.readDouble(json['period_ms']);
    if (period != null &&
        period >= minPeriodMs &&
        period <= maxPeriodMs &&
        period == period.round()) {
      return BobUpDown(periodMs: period.toInt());
    }
    return BobUpDown();
  }

  @override
  Map<String, dynamic> get parameters => {
    if (periodMs != null) 'period_ms': periodMs,
  };

  @override
  Map<String, dynamic> toJson() => {...super.toJson(), ...parameters};
}

class StopCommand extends Command {
  StopCommand() : super(id: 'stop');
}

enum WalkGait {
  tripod('Tripod', 10, 22, 26),
  bipod('Bipod', 6, 14, 16);

  final String label;
  final int minLiftMm;
  final int defaultLiftMm;
  final int maxLiftMm;

  const WalkGait(
    this.label,
    this.minLiftMm,
    this.defaultLiftMm,
    this.maxLiftMm,
  );
}

class Move extends Command {
  static const defaultStepMs = 450;
  static const minStepMs = 300;
  static const maxStepMs = 800;

  final double? theta;
  final int? stepMs;
  final int? liftMm;
  final WalkGait gait;

  Move({this.theta, this.stepMs, this.liftMm, this.gait = WalkGait.tripod})
    : super(id: gait == WalkGait.bipod ? 'move_bipod' : 'move');

  factory Move.fromJson(Map<String, dynamic> json) {
    final step = Command.readDouble(json['step_ms']);
    final gait = json['id'] == 'move_bipod' ? WalkGait.bipod : WalkGait.tripod;
    final lift = Command.readDouble(json['lift_mm']);
    return Move(
      theta: Command.readDouble(json['theta']),
      gait: gait,
      liftMm:
          lift != null &&
              lift >= gait.minLiftMm &&
              lift <= gait.maxLiftMm &&
              lift == lift.round()
          ? lift.toInt()
          : null,
      stepMs:
          step != null &&
              step >= minStepMs &&
              step <= maxStepMs &&
              step == step.round()
          ? step.toInt()
          : null,
    );
  }

  @override
  Map<String, dynamic> toJson() => {...super.toJson(), ...parameters};

  @override
  Map<String, dynamic> get parameters => {
    if (theta != null) 'theta': theta,
    if (stepMs != null) 'step_ms': stepMs,
    if (liftMm != null) 'lift_mm': liftMm,
  };
}
