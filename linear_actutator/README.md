# Linear Actuator Control Notes

This folder contains an Arduino + Python workflow for controlling a linear actuator with an IBT-2 H-bridge and a draw-wire potentiometer sensor.

## What is here

- `linear_act_dc_potentiometer/linear_act_dc_potentiometer.ino`: Main Arduino control loop.
- `linear_act_dc_potentiometer/keyboard_direction_serial.py`: Python CLI to send target values over serial.
- `linear_act_dc_potentiometer/ref_images/`: Wiring and reference images.
- `observations.md`: Testing notes and links.

## Current control behavior

- Sensor input: `A3`.
- Motor PWM pins: `RPWM=5`, `LPWM=6`.
- Target is runtime-configurable over serial (newline-terminated integer).
- Valid target range: `0..1023`.
- Initial target at boot: `250`.
- Deadband around target: `30` ADC counts.
- Drive power: `DRIVE_PWM = 70`.
- Sampling interval: `100 ms`.
- Filtering: 7-sample median (`MEDIAN_SAMPLES = 7`) to reduce noise.

## Serial protocol

Send from host to Arduino:

- `"<number>\n"` where number is `0..1023`.

Arduino responses:

- `READY` on startup.
- `TARGET_SET <value>` when a target is accepted.
- `INVALID_TARGET` when input is malformed/out of range.

## How to run

1. Open and upload `linear_act_dc_potentiometer/linear_act_dc_potentiometer.ino` to the Arduino.
2. Install Python dependency:
   - `pip install pyserial`
3. Run the sender script (replace COM port depending on the arduino IDE detected port):
   - `python linear_act_dc_potentiometer/keyboard_direction_serial.py --port COM5`
4. At prompt `target>`, enter a number and press Enter.
5. Enter `q` to quit.

## Tuning guide

For faster movement:

- Increase `DRIVE_PWM`.
- Decrease `SAMPLE_INTERVAL_MS`.

For less noisy motion:

- Keep/increase `MEDIAN_SAMPLES` (costs response speed).
- Increase `TARGET_DEADBAND` slightly if oscillating near setpoint.

## Known limitations

- Control is bang-bang (full on/off), not PID.
- No hard-limit switch handling in code.
- Behavior depends on wiring polarity; toggle `invertDirection` if direction is reversed.
