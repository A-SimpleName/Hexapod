import 'package:app/services/movement_limits.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class AdminSettingsPage extends StatefulWidget {
  final MovementLimits initialLimits;
  final Future<void> Function(MovementLimits) onSave;

  const AdminSettingsPage({
    super.key,
    required this.initialLimits,
    required this.onSave,
  });

  @override
  State<AdminSettingsPage> createState() => _AdminSettingsPageState();
}

class _AdminSettingsPageState extends State<AdminSettingsPage> {
  final _form = GlobalKey<FormState>();
  late bool _bipodEnabled = widget.initialLimits.bipodEnabled;
  bool _saving = false;
  late final _fields = {
    for (final control in MotionControl.values)
      control: (
        min: TextEditingController(
          text: '${widget.initialLimits.rangeFor(control).min}',
        ),
        max: TextEditingController(
          text: '${widget.initialLimits.rangeFor(control).max}',
        ),
      ),
  };

  @override
  void dispose() {
    for (final fields in _fields.values) {
      fields.min.dispose();
      fields.max.dispose();
    }
    super.dispose();
  }

  Future<void> _save() async {
    FocusScope.of(context).unfocus();
    final invalid = _form.currentState!.validateGranularly();
    if (invalid.isNotEmpty) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) Scrollable.ensureVisible(invalid.first.context);
      });
      return;
    }
    final limits = MovementLimits(
      bipodEnabled: _bipodEnabled,
      ranges: {
        for (final entry in _fields.entries)
          entry.key: (
            min: int.parse(entry.value.min.text),
            max: int.parse(entry.value.max.text),
          ),
      },
    );
    setState(() => _saving = true);
    try {
      await widget.onSave(limits);
      if (mounted) Navigator.pop(context, limits);
    } catch (error) {
      debugPrint('Could not save movement limits: $error');
      if (!mounted) return;
      setState(() => _saving = false);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Could not save admin settings.')),
      );
    }
  }

  Widget _boundField(MotionControl control, {required bool isMinimum}) {
    final fields = _fields[control]!;
    final allowed = control.firmwareRange;
    final bound = isMinimum ? 'min' : 'max';
    return TextFormField(
      key: ValueKey('${control.name}-$bound'),
      controller: isMinimum ? fields.min : fields.max,
      enabled: !_saving,
      keyboardType: TextInputType.number,
      inputFormatters: [FilteringTextInputFormatter.digitsOnly],
      decoration: InputDecoration(
        labelText: isMinimum ? 'Minimum' : 'Maximum',
        suffixText: control.unit,
        border: const OutlineInputBorder(),
      ),
      validator: (text) {
        final value = int.tryParse(text ?? '');
        if (value == null || value < allowed.min || value > allowed.max) {
          return 'Use ${allowed.min}–${allowed.max}';
        }
        final minimum = int.tryParse(fields.min.text);
        if (!isMinimum && minimum != null && value < minimum) {
          return 'Below minimum';
        }
        return null;
      },
    );
  }

  @override
  Widget build(BuildContext context) => PopScope(
    canPop: !_saving,
    child: Scaffold(
      appBar: AppBar(
        title: const Text('Admin settings'),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 16),
            child: FilledButton(
              onPressed: _saving ? null : _save,
              child: const Text('Save'),
            ),
          ),
        ],
      ),
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 720),
            child: Form(
              key: _form,
              child: SingleChildScrollView(
                padding: const EdgeInsets.fromLTRB(20, 8, 20, 24),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    SwitchListTile(
                      contentPadding: EdgeInsets.zero,
                      title: const Text('Enable Bipod'),
                      value: _bipodEnabled,
                      onChanged: _saving
                          ? null
                          : (enabled) =>
                                setState(() => _bipodEnabled = enabled),
                    ),
                    const Divider(),
                    Text(
                      'Slider ranges',
                      style: Theme.of(context).textTheme.titleLarge,
                    ),
                    const Text('Shorter times make the motion faster.'),
                    const SizedBox(height: 16),
                    LayoutBuilder(
                      builder: (context, constraints) {
                        const gap = 24.0;
                        final width = constraints.maxWidth >= 520
                            ? (constraints.maxWidth - gap) / 2
                            : constraints.maxWidth;
                        return Wrap(
                          spacing: gap,
                          runSpacing: 20,
                          children: [
                            for (final control in MotionControl.values)
                              SizedBox(
                                width: width,
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(
                                      control.label,
                                      style: Theme.of(
                                        context,
                                      ).textTheme.titleMedium,
                                    ),
                                    Text(
                                      'Firmware range ${control.firmwareRange.min}–${control.firmwareRange.max} ${control.unit}',
                                    ),
                                    const SizedBox(height: 8),
                                    Row(
                                      crossAxisAlignment:
                                          CrossAxisAlignment.start,
                                      children: [
                                        Expanded(
                                          child: _boundField(
                                            control,
                                            isMinimum: true,
                                          ),
                                        ),
                                        const SizedBox(width: 12),
                                        Expanded(
                                          child: _boundField(
                                            control,
                                            isMinimum: false,
                                          ),
                                        ),
                                      ],
                                    ),
                                  ],
                                ),
                              ),
                          ],
                        );
                      },
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    ),
  );
}
