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
      case 'move':
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
        ? offsetsRaw.map((value) => Command.readDouble(value)).whereType<double>().toList()
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
  Map<String, dynamic> get parameters => {
    if (speed != null) 'speed': speed,
  };
}

class StopCommand extends Command {
  StopCommand() : super(id: 'stop');
}

class Move extends Command {
  final double? theta;

  Move({this.theta}) : super(id: 'move');

  factory Move.fromJson(Map<String, dynamic> json) {
    return Move(theta: Command.readDouble(json['theta']));
  }

  @override
  Map<String, dynamic> toJson() => {
    'id': id,
    if (theta != null) 'theta': theta,
  };

  @override
  Map<String, dynamic> get parameters => {
    if (theta != null) 'theta': theta,
  };
}
