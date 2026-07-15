# AGENTS.md

## Scope
- This git repo is a loose workspace with several unrelated projects. There is no single root build, test, or lint command.
- Check for a deeper `AGENTS.md` before editing inside a subproject. Verified local files exist in:
  - `linear_actuator/documents/AGENTS.md`
  - `simple_robot_test/URDF/pyrob/simple_robot/AGENTS.md`

## Repo Shape
- `linear_actuator/`: Arduino firmware plus a Python FastAPI bridge for serial-controlled actuators.
- `robot_ik/path_determinations/`: standalone Python analysis scripts for path JSON and URDF data.
- `robot_ik/agb_robot/` and `robot_ik/agb_robot_description/`: mesh + URDF assets for the same robot, but not identical.
- `simple_robot_test/URDF/pyrob/simple_robot/`: PyBullet URDF sandbox with its own local instructions.

## Verified Commands
- Root Python deps file is `venv_requirements.txt`; it matches the linear-actuator FastAPI stack (`fastapi`, `uvicorn`, `pyserial`, `requests`), not the whole workspace.
- Linear actuator controller from repo root:
  - `python linear_actuator\controller_py\main.py --port COM5`
  - `python linear_actuator\controller_py\main.py --api-test-only --api-port 7500`
  - `python linear_actuator\controller_py\start_mapped_servers.py`
  - `python linear_actuator\controller_py\start_mapped_servers.py --api-test-only --only API00 API01`
- Arduino sketch mentioned by the root README:
  - `linear_actuator\linear_act_dc_potentiometer\linear_act_dc_potentiometer.ino`
- `robot_ik/path_determinations` scripts are direct-entry Python files, not a package:
  - `python robot_ik\path_determinations\joint_speeds.py`
  - `python robot_ik\path_determinations\plot_joint_positions.py`

## `robot_ik` Gotchas
- `robot_ik/path_determinations/joint_speeds.py` writes into a new timestamped directory on every run: `output_YYYYMMDDHHMM/{en,jp}/...`. Do not assume it updates files in place.
- That script also copies `CSV_COLUMNS_EXPLANATION.md` and `CSV_COLUMNS_EXPLANATION_JP.md` into each output language directory.
- Default analysis inputs in `path_determinations` are local files: `path.json` and `my_robot.urdf`.
- There are multiple URDF variants for the same robot with materially different joint semantics:
  - `robot_ik/agb_robot/agb_robot/agb_robot.urdf` uses revolute joints with explicit limits for the rotary axes.
  - `robot_ik/agb_robot_description/agb_robot_260223/agb_robot_260223.urdf` uses `continuous` joints for those same axes and omits those limits.
- Do not “sync” those URDFs blindly. First confirm which consumer needs limited joints versus continuous joints, because analysis results change.

## Working Norms
- Prefer per-project smoke checks over repo-wide commands; there is no verified root test suite or CI workflow.
- Treat generated analysis outputs under `robot_ik/path_determinations/output_*` as run artifacts unless the task explicitly asks to update checked-in results.
