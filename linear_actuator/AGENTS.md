# Project Scope

- Treat `linear_actuator/` as the development scope; do not modify parent/root repo files unless the user explicitly expands scope.
- This project is a C# rewrite target; the verified source behavior is the Python + Arduino implementation in `./lin_act_controller_modules/`.
- Current C# solution scaffold: `LinearActuator.slnx` with `LinearActuator.App`, `LinearActuator.Core`, `LinearActuator.Infrastructure`, and `LinearActuator.Tests`.

## Archive Behavior To Preserve

- Python API entrypoint: `archive/controller_py/main.py`.
- Arduino firmware entrypoint: `lin_act_controller_modules/lin_act_controller_modules.ino`.
- The compatibility API exposes `GET /actuators` and `POST /actuators` for module `M01` with the flat four-actuator schema: `a1_current`, `a1_target`, ..., `a4_current`, `a4_target`.
- The bundle API exposes `GET /actuator-bundles` and `POST /actuator-bundles` with `{ "modules": { "M01": ActuatorState, ..., "M10": ActuatorState } }`.
- The API port is fixed at `127.0.0.1:7500`; do not reintroduce multiple API ports unless explicitly requested.
- The system has 10 serial-controlled modules, each with 4 actuators. WPF/SQLite serial toggles decide which modules send commands to COM ports.
- Server state is authoritative for targets; serial telemetry updates only `*_current` fields.
- Default target is `50` for all four actuators.
- Serial command sent to Arduino is CSV, not JSON: `T,a1,a2,a3,a4\n`.
- Current Arduino parser accepts targets in `0..800`; trust `lin_act_controller_modules/serial_protocol.h` over older prose if ranges conflict.
- Arduino telemetry/error output is JSON lines; baud rate is `9600`.

## C# Rewrite Style

- Separate files when responsibilities differ, but do not fragment code into tiny abstractions.
- Do not extract a function/method unless it is called from more than one place or materially improves readability.
- Prefer readable, direct code over premature abstraction.

## C# Commands

- Build: `dotnet build LinearActuator.slnx`.
- Test: `dotnet test LinearActuator.slnx`.
- Run the WPF host: `dotnet run --project LinearActuator.App`.
- Publish single-file Windows exe: `dotnet publish LinearActuator.App -c Release -r win-x64 --self-contained true /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true`.

## Useful Archive Commands

- API-only smoke test, no Arduino required: `python archive\controller_py\main.py --api-test-only --api-port 7500`.
- Serial mode example: `python archive\controller_py\main.py --port COM5 --baud 9600 --api-host 127.0.0.1 --api-port 7500`.
- Start mapped servers from `archive/controller_py/port_map.json`: `python archive\controller_py\start_mapped_servers.py --api-test-only` or omit `--api-test-only` for serial mode.
- Kill mapped API listeners from this folder: `powershell -ExecutionPolicy Bypass -File archive\controller_py\kill_mapped_ports.ps1 -MapFile archive\controller_py\port_map.json`.

## C# Re-write Components

1. .NET WebAPI with Entity Framework SQLite Database
    - Database Models
    - DTO Models
    - Arduino Serial Read/Write
2. WPF UI for Arduino port mapping.
