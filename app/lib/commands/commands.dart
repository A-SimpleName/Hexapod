abstract class Command {
  final String id;

  Command({required this.id});

  Map<String, dynamic> toJson() => {'id': id};
}

class WaveLegs extends Command {
  final List<double>? legOffsets;
  final double? speed;

  WaveLegs({this.legOffsets, this.speed}) : super(id: 'wave_legs');

}

class CircleJerk extends Command {
  final double? speed;

  CircleJerk({this.speed}) : super(id: 'circle_jerk');
}

class StopCommand extends Command {
  StopCommand() : super(id: 'stop');
}