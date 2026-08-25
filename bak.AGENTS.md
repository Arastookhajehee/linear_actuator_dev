# AGENTS.md

## Scope

- This repo is a loose workspace with separated projects; there is no verified root build, test, lint, or CI workflow.
- Check for a deeper `AGENTS.md` before editing a subproject. Verified local files: `linear_actuator/AGENTS.md` and `simple_robot_test/URDF/pyrob/simple_robot/AGENTS.md`.
- Several linear-actuator files that the root README names at `linear_actuator/...` are currently only present under `linear_actuator/archive/...`; `.gitignore` ignores any `archive/` directory, so verify the intended tracked location before moving or editing actuator code.

## Root Tooling

- No root package manifest, solution, Makefile, task runner, pyproject, test config, or GitHub workflow is present.
- `.markdownlint.json` enables markdownlint defaults and disables only `MD013` line length.
- `venv_requirements.txt` is UTF-16LE and matches the FastAPI/uvicorn/pyserial/requests actuator controller stack, not the whole workspace.

## Linear Actuator

- Current available Python controller entrypoints are in `linear_actuator/archive/controller_py/`.
- Run one serial-backed API server from repo root with `python linear_actuator\archive\controller_py\main.py --port COM5`; serial mode requires `--port` and defaults to baud `9600`.
- Run without Arduino/serial with `python linear_actuator\archive\controller_py\main.py --api-test-only --api-port 7500`; `--rest-test` is an alias.
- Launch mapped servers with `python linear_actuator\archive\controller_py\start_mapped_servers.py`; `--api-test-only` avoids serial and `--only API01` restricts keys. The local map currently contains only `API01` mapping `COM4` to API port `7500`.
- Optional mapped-port cleanup script: `powershell -ExecutionPolicy Bypass -File linear_actuator\archive\controller_py\kill_mapped_ports.ps1`.
- Current Arduino sketch is `linear_actuator/archive/lin_act_controller_modules/lin_act_controller_modules.ino`; it includes `Arduino_JSON` and calls `Serial.begin(9600)`.

## `robot_ik`

- `robot_ik/path_determinations` scripts are direct-entry Python files, not a package. Run them from `robot_ik/path_determinations` unless passing explicit `--path` and `--urdf` paths, because defaults like `path.json` and `my_robot.urdf` are relative to the current working directory.
- Useful focused commands from `robot_ik/path_determinations`: `python joint_speeds.py`, `python plot_joint_positions.py`, `python batch_plot_joint_positions.py`, and `python count_steps.py path.json`.
- `joint_speeds.py` writes a new `output_YYYYMMDDHHMM/{en,jp}/...` tree under the parent of `--csv` on every run and copies the English/Japanese CSV explanation markdown into those language folders.
- `plot_joint_positions.py` writes `joint_positions_stacked.jpg` by default; `batch_plot_joint_positions.py` reads `plot_json/*.json`, writes `plot_json/jpg/*.jpg`, and skips existing JPGs.
- URDF variants are materially different: `robot_ik/agb_robot/agb_robot/agb_robot.urdf` uses limited `revolute` rotary joints, while `robot_ik/agb_robot_description/agb_robot_260223/agb_robot_260223.urdf` uses `continuous` rotary joints without those limits. Do not sync them blindly.
- `robot_ik/quick_start.md` is ROS/MoveIt quick-start prose for an external workspace (`~/repos/my_robot/ws_robot`), not a runnable command set for this repo checkout.

## Simple Robot Sandbox

- `simple_robot_test/URDF/pyrob/simple_robot/` is a PyBullet URDF sandbox with its own local instructions.
- Run `python hello_bullet.py` from that directory; it uses `p.GUI`, steps for 10000 iterations, then waits for keyboard input before disconnecting.

## Working Norms

- Prefer per-project smoke checks over repo-wide commands.
- Treat generated analysis outputs under `robot_ik/path_determinations/output_*` as run artifacts unless the task explicitly asks to update checked-in results.
