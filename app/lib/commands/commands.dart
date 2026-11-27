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
      case 'rotate':
        return Rotate.fromJson(json);
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
    if (value is num) {
      return value.toDouble();
    }
    if (value is String) {
      return double.tryParse(value);
    }
    return null;
  }

  static int? _readBoundedInt(dynamic value, int min, int max) {
    final number = readDouble(value);
    if (number != null &&
        number >= min &&
        number <= max &&
        number == number.round()) {
      return number.toInt();
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
        ? offsetsRaw.map(Command.readDouble).whereType<double>().toList()
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
  static const defaultPeriodMs = 2000;
  static const minPeriodMs = 400;
  static const maxPeriodMs = 4800;

  final int? periodMs;

  BobUpDown({this.periodMs}) : super(id: 'bob_up_down');

  factory BobUpDown.fromJson(Map<String, dynamic> json) => BobUpDown(
    periodMs: Command._readBoundedInt(
      json['period_ms'],
      minPeriodMs,
      maxPeriodMs,
    ),
  );

  @override
  Map<String, dynamic> get parameters => {
    if (periodMs != null) 'period_ms': periodMs,
  };

  @override
  Map<String, dynamic> toJson() => {...super.toJson(), ...parameters};
}

enum RotationDirection {
  left(-1),
  right(1);

  const RotationDirection(this.wireValue);

  final int wireValue;
}

class Rotate extends Command {
  static const defaultStepMs = 400;
  static const minStepMs = 200;
  static const maxStepMs = 800;

  final RotationDirection direction;
  final int? stepMs;

  Rotate({this.direction = RotationDirection.right, this.stepMs})
    : super(id: 'rotate');

  factory Rotate.fromJson(Map<String, dynamic> json) => Rotate(
    direction: switch (Command.readDouble(json['direction'] ?? 1)) {
      -1 => RotationDirection.left,
      1 => RotationDirection.right,
      _ => throw const FormatException('Rotation direction must be -1 or 1'),
    },
    stepMs: Command._readBoundedInt(json['step_ms'], minStepMs, maxStepMs),
  );

  @override
  Map<String, dynamic> get parameters => {
    'direction': direction.wireValue,
    if (stepMs != null) 'step_ms': stepMs,
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
  static const defaultStepMs = 300;
  static const minStepMs = 100;
  static const maxStepMs = 800;

  final double? theta;
  final int? stepMs;
  final int? liftMm;
  final WalkGait gait;

  Move({this.theta, this.stepMs, this.liftMm, this.gait = WalkGait.tripod})
    : super(id: gait == WalkGait.bipod ? 'move_bipod' : 'move');

  factory Move.fromJson(Map<String, dynamic> json) {
    final step = Command._readBoundedInt(json['step_ms'], minStepMs, maxStepMs);
    final gait = json['id'] == 'move_bipod' ? WalkGait.bipod : WalkGait.tripod;
    final lift = Command._readBoundedInt(
      json['lift_mm'],
      gait.minLiftMm,
      gait.maxLiftMm,
    );
    return Move(
      theta: Command.readDouble(json['theta']),
      gait: gait,
      liftMm: lift,
      stepMs: step,
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
