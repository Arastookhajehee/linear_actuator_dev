# Linear Actuator Control Notes

This folder contains an Arduino + Python workflow for controlling a linear actuator with an IBT-2 H-bridge and a draw-wire potentiometer sensor.

## What is here

- Main Arduino control loop.
- Python controller to send target values over serial.
- Wiring and reference images (to be updated)

## Current control behavior

![controller Data Flow Diagram](./linear_actuator/linear_act_dc_potentiometer/data_flow_diagram.png)

- Valid target range: `0..1023`.
- Initial target at boot: `50`.
- Deadband around target: `10` ADC counts.
- Drive power: `DRIVE_PWM = 70`.
- Sampling interval: `100 ms`.
- Filtering: 7-sample median (`MEDIAN_SAMPLES = 7`) to reduce noise.

## Serial protocol

1. Install the `https://github.com/arduino-libraries/Arduino_JSON` library.
   - The package is available via the Arduino IDE Package manager.
2. Verify and upload code to Arduino Mega as usual.

## How to run python linear actuator controller

1. Open and upload `linear_actuator\linear_act_dc_potentiometer\linear_act_dc_potentiometer.ino` to the Arduino.
2. Activate/create python virtual environment `venv`
3. Install Python dependencies:
   - `pip install -r ./venv_requirements.txt`
4. Run one controller server (serial mode).
   - Default serial baud is `9600` (matches Arduino `Serial.begin(9600)`).
   - Example: `python linear_actuator\controller_py\main.py --port COM5`
   - Optional: override bind host/port and baud:
     - `python linear_actuator\controller_py\main.py --port COM5 --baud 9600 --api-host 127.0.0.1 --api-port 7500`
5. Or run one API test-only server (no Arduino/serial):
   - `python linear_actuator\controller_py\main.py --api-test-only --api-port 7500`
   - `--rest-test` is still supported as an alias.
6. Or launch all mapped servers from `linear_actuator\controller_py\port_map.json`:
   - Serial mode: `python linear_actuator\controller_py\start_mapped_servers.py`
   - API test-only mode: `python linear_actuator\controller_py\start_mapped_servers.py --api-test-only`
   - Launch a subset: `python linear_actuator\controller_py\start_mapped_servers.py --only API00 API01`
7. Optional cleanup script for mapped API ports:
   - `powershell -ExecutionPolicy Bypass -File linear_actuator\controller_py\kill_mapped_ports.ps1`

## REST endpoints

- `GET /actuators` -> returns current actuator state JSON
- `POST /actuators` -> updates actuator targets (and returns current state)

POST body example:

```json
{
  "a1_target": 30,
  "a2_target": 50,
  "a3_target": 90,
  "a4_target": 120
}
```
