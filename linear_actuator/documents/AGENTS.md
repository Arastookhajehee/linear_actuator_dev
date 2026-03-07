# AGENTS.md

Agent guidance for `C:\Users\akhaj\source\repos\agb\linear_actuator`.

## Scope and Intent

- This repository contains:
- Arduino firmware in `linear_act_dc_potentiometer/linear_act_dc_potentiometer.ino`
- A Python host utility in `linear_act_dc_potentiometer/keyboard_direction_serial.py`
- Project notes in `observations.md`, `project_plan.md`, and `TODO.md`
- Main goal: safely drive a linear actuator toward a serial-configured target.

## Repository Rules Sources

- Cursor rules:
- No `.cursorrules` file found.
- No `.cursor/rules/` directory found.
- Copilot rules:
- No `.github/copilot-instructions.md` file found.
- Therefore, rely on this file plus existing in-repo coding patterns.

## Working Directories

- Repo root: `C:\Users\akhaj\source\repos\agb\linear_actuator`
- Firmware directory: `C:\Users\akhaj\source\repos\agb\linear_actuator\linear_act_dc_potentiometer`

## Build / Lint / Test Commands

- Run commands from repo root unless noted.

### Firmware (Arduino)

- Install/check Arduino CLI:
- `arduino-cli version`
- Discover boards:
- `arduino-cli board list`
- Update board index:
- `arduino-cli core update-index`
- Install AVR core (for Uno/Nano class boards):
- `arduino-cli core install arduino:avr`
- Compile firmware (Uno example):
- `arduino-cli compile --fqbn arduino:avr:uno linear_act_dc_potentiometer`
- Upload firmware (replace COM port):
- `arduino-cli upload -p COM5 --fqbn arduino:avr:uno linear_act_dc_potentiometer`
- Open serial monitor:
- `arduino-cli monitor -p COM5 -c baudrate=9600`

### Python Host Script

- Create virtual environment (PowerShell):
- `python -m venv .venv`
- `.\.venv\Scripts\Activate.ps1`
- Install dependency:
- `pip install pyserial`
- Run host script:
- `python linear_act_dc_potentiometer/keyboard_direction_serial.py --port COM5 --baud 9600`

### Lint / Format (Python)

- There is no enforced lint config currently, but preferred checks are:
- `ruff check linear_act_dc_potentiometer`
- `black --check linear_act_dc_potentiometer`
- Optional type check:
- `mypy linear_act_dc_potentiometer/keyboard_direction_serial.py`
- If tools are missing, install:
- `pip install ruff black mypy`

### Tests

- Current state: no automated test suite is present yet.
- If adding tests, use `pytest` and place tests under `tests/`.
- Recommended full run:
- `pytest -q`
- Recommended single-test run (node id):
- `pytest -q tests/test_serial_protocol.py::test_accepts_valid_target`
- Recommended single-test run (pattern):
- `pytest -q -k valid_target`
- Install pytest if needed:
- `pip install pytest`

## Single-Test Guidance (Important)

- Prefer node-id syntax for deterministic selection:
- `pytest path/to/test_file.py::test_name`
- For parameterized tests, include case id when needed:
- `pytest path/to/test_file.py::test_name[param_case]`
- Use `-k` only for quick filtering during local iteration.
- Keep single tests fast and hardware-independent when possible.

## Code Style Guidelines

Follow existing style in each language; do not mix conventions.

### Python Style (`keyboard_direction_serial.py`)

- Follow PEP 8 with 4-space indentation.
- Keep lines Black-compatible (88 chars target).
- Use module docstrings for script purpose and usage.
- Use explicit type hints on all functions.
- Keep functions small and focused (`send_target`, `print_pending_lines`, etc.).

### Python Imports

- Group imports in this order:
- Standard library
- Third-party packages
- Local modules
- Keep one import per line unless tightly related.
- Avoid wildcard imports.

### Python Naming

- `snake_case` for variables/functions.
- `UPPER_SNAKE_CASE` for constants.
- Use descriptive names for hardware/protocol values (`window_seconds`, `target`).
- Keep CLI argument names explicit (`--port`, `--baud`, `--timeout`).

### Python Error Handling

- Validate user input early and print actionable messages.
- Catch narrow exceptions (`ValueError`, `EOFError`, `KeyboardInterrupt`).
- Use `try/finally` for serial cleanup (`ser.close()`).
- Decode serial input with replacement fallback to avoid crashes on bad bytes.

### Arduino/C++ Style (`.ino`)

- Keep constants near top of file.
- Use `const` for pin IDs and fixed tuning values.
- Preserve current brace and indentation style used in file.
- Prefer small helper functions over large `loop()` bodies.
- Keep motor control logic explicit and easy to audit.

### Arduino Naming

- Pin and fixed config values use `UPPER_SNAKE_CASE`-like names.
- Runtime state uses descriptive lower/camel names (`targetValue`, `lastSampleMs`).
- Function names use verb-first camelCase (`handleSerialInput`, `stopMotor`).

### Arduino Error Handling and Safety

- Validate all serial target inputs (`0..1023`, numeric only).
- Reject malformed input with a clear serial status line (`INVALID_TARGET`).
- Default to safe state on uncertainty: stop motor outputs.
- Keep deadband checks before drive commands to reduce oscillation.
- Maintain overflow handling for serial buffers.

## Behavioral Expectations for Agents

- Prefer minimal, targeted changes over broad refactors.
- Preserve serial protocol strings unless explicitly updating both sides.
- When changing control constants, document reason in commit/notes.
- Avoid introducing hardware-specific assumptions without comments.
- Do not silently change baud rate, pin mappings, or direction polarity.

## Documentation Expectations

- Update `observations.md` after meaningful hardware tests.
- Record date/time, setup, command used, and observed behavior.
- If calibrating ADC-to-mm mapping, include raw points and fitted method.

## Pre-PR / Pre-Commit Checklist

- Firmware compiles with `arduino-cli compile` for target board.
- Python script runs and connects with expected CLI arguments.
- Lint/format checks pass (or document why not run).
- Any new tests pass locally; include at least one focused test where feasible.
- Notes/docs updated when behavior or tuning changes.

## Known Gaps

- No CI pipeline detected.
- No formal formatter/linter configuration files detected.
- No automated tests currently checked in.
- If you add tooling configs, keep them lightweight and repo-local.
