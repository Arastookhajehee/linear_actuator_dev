From [[Robotics]]

# Setting Up a Server to Talk with MoveIt 2

**Important**: code snippets in early steps are incremental learning checkpoints.
For latest copy/paste-ready module code, always use the `## Appendix` section as the source of truth.

- Confirm prerequisites and target behavior
- Define server contract before coding
- Create ROS 2 package for bridge node
- Implement MoveIt 2 control layer
- Add external server interface
- Add validation and safety guards
- Add status reporting and observability
- Create launch flow and startup scripts
- Run end-to-end tests with a simple client
- Harden for real usage
- Wire `/plan` to MoveIt planning backend
- Map request payload to real planning targets
- Return real planning output to client
- Add `/current` endpoint via MoveIt planning scene
- Verify real planner integration end-to-end

## Quick Start Scripts

**This works ONLY if the name of the robot is set to `my_robot_name` and the installation has successfully completed.**

- Terminal A: launch MoveIt demo for your robot config.
- Terminal B: run HTTP solver server.

A. Terminal A commands (MoveIt):

```bash
export ROBOTNAME=my_robot_name
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
source ./install/setup.bash
ros2 launch ${ROBOTNAME}_config demo.launch.py
```

B. Terminal B commands (server):

```bash
export ROBOTNAME=my_robot_name
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
source ./install/setup.bash
ros2 run moveit_solver_server server
```

---

## Step 1: Confirm prerequisites and target behavior

**Locked setup**:
- Environment: WSL Ubuntu 22.04 with ROS 2 Humble and MoveIt 2.
- Robot status: built and launchable MoveIt simulation from URDF, with joint limits and self-collision working in RViz.
- Transport: HTTP server.
- Client: Postman.
- `frame_id`: required in every request.
- Planning timeout: 30 seconds.
- Execution: not required for now (planning only).

**Start directory rule (from scratch)**:
- Use one workspace root for all commands in this checklist.
- In your current setup, use: `~/repos/my_robot/ws_robot`
- `WORKSPACE_ROOT` must point to the directory that contains `src`, `build`, `install`, and `log`.
- `export WORKSPACE_ROOT=...` is session-only (not permanent unless added to `~/.bashrc`).

```bash
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
mkdir -p "$WORKSPACE_ROOT"
cd "$WORKSPACE_ROOT"
pwd
ls
```

Expected output:
- `pwd` prints `/home/<user>/repos/my_robot/ws_robot`
- `ls` includes `src`, `build`, `install`, and `log`

Optional permanent setup:

```bash
echo 'export WORKSPACE_ROOT=~/repos/my_robot/ws_robot' >> ~/.bashrc
source ~/.bashrc
```

**Possible problems**:
- ROS 2/MoveIt 2 not installed or partial install.
- URDF/SRDF package path is unknown.
- Scope is unclear (joint goals only vs Cartesian planning vs execute).
- Commands run from the wrong directory.

---

## Step 2: Define the server contract before coding

**Request schema (MVP, planning only)**:

```json
{
  "frame_id": "world",
  "target": [x, y, z, qa, qb, qc, qd],
  "current": [a1, a2, a3, a4, a5, a6]
}
```

**Unit convention (server canonical, MoveIt-aligned)**:
- Joint rotations in `current`: radians.
- Prismatic joints in `current`: meters.
- `target` position (`x`, `y`, `z`): meters.
- `target` orientation (`qa`, `qb`, `qc`, `qd`): normalized quaternion.
- Client must send numbers already in these conventions.

**MVP response expectation**:
- HTTP `200` with JSON for valid requests.
- Response includes planning status (`plan_success: true|false`).
- Response includes actionable error details when planning fails.
- No execution endpoint in MVP.

**Possible problems**:
- Mixed units (degrees/radians or mm/meters).
- Missing `frame_id` for pose targets.
- Non-normalized quaternion causes invalid target orientation.

---

## Step 3: Create a ROS 2 package for the bridge node

**Goal**: create one ROS 2 package for the HTTP bridge and keep all solver-server logic inside it.

**Checklist**:

1. Create workspace layout and package.

```bash
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
mkdir -p "$WORKSPACE_ROOT/src"
cd "$WORKSPACE_ROOT/src"
source /opt/ros/humble/setup.bash
ros2 pkg create moveit_solver_server --build-type ament_python --dependencies rclpy geometry_msgs sensor_msgs std_msgs moveit_msgs
```

2. Create and activate virtual environment `venv` at workspace root.

```bash
cd "$WORKSPACE_ROOT"
python3 -m venv venv
source ./venv/bin/activate
python -m pip install --upgrade pip
```

3. Install HTTP dependencies in `venv`.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
python -m pip install --upgrade pip
python -m pip install fastapi uvicorn pydantic
```

4. Build package and source overlay.

```bash
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
```

5. Verify package visibility.

```bash
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
source ./install/setup.bash
ros2 pkg list | rg moveit_solver_server
```

**Expected result**:
- Build completes with no dependency errors.
- Step 3.5 prints one line: `moveit_solver_server`.

**Possible problems**:
- Missing dependencies in `package.xml`.
- Workspace overlay not sourced.
- `Permission denied` while installing Python packages.

**If you hit `Permission denied`**:

```bash
cd "$WORKSPACE_ROOT"
sudo chown -R $USER:$USER .
source ./venv/bin/activate
python -m pip install --upgrade pip
python -m pip install fastapi uvicorn pydantic
```

---

## Step 4: Implement MoveIt 2 control layer in the node

**Goal**: create the Python module used by `ros2 run` and keep planning-only behavior.

**Checklist**:

1. Enter package directory.

```bash
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT/src/moveit_solver_server"
```

2. Ensure module files exist.

```bash
mkdir -p moveit_solver_server
touch moveit_solver_server/__init__.py
```

3. Create `moveit_solver_server/server.py` with health + `POST /plan` endpoints.

```python
from fastapi import FastAPI
from pydantic import BaseModel, Field
import uvicorn


class PlanRequest(BaseModel):
    frame_id: str
    target: list[float] = Field(min_length=7, max_length=7)
    current: list[float] = Field(min_length=6, max_length=6)


app = FastAPI()


@app.get("/health")
def health():
    return {"ok": True}


@app.post("/plan")
def plan(request: PlanRequest):
    return {
        "plan_success": True,
        "message": "Plan endpoint reached",
        "frame_id": request.frame_id,
    }


def main():
    uvicorn.run(app, host="172.21.1.108", port=8000)
```

## Error Handling

### Current execution issue summary


- Planning works end-to-end: `/plan` returns success and RViz can animate the planned trajectory.
- Execution fails in both RViz (`Execute`) and API (`POST /execute`) with MoveIt error code `-4` (`CONTROL_FAILED`).
- `/execute_trajectory` action is available from `/move_group`, so MoveIt execute pipeline is present.
- `/arm_controller/follow_joint_trajectory` has clients but no action server (`Action servers: 0`), so there is no controller backend to execute trajectory commands.
- Spawner nodes (`/spawner_arm_controller`, `/spawner_joint_state_broadcaster`) are running, but real `controller_manager` node is not healthy/serving normal APIs.
- Only `/controller_manager/list_controllers` appears and is non-responsive (`waiting for service to become available...`), indicating broken/incomplete ros2_control bringup.

### Root cause (most likely)

- MoveIt configuration points to an arm trajectory controller (`arm_controller`), but ros2_control runtime is not fully up.
- Without a healthy `controller_manager` and loaded active controllers, no FollowJointTrajectory action server is created.
- Result: planning succeeds, execution is always rejected/fails (`CONTROL_FAILED`).

### Symptoms and what they mean

- `POST /plan` success + RViz animation works:
  - planning stack and kinematics are OK.
- `POST /execute` returns `error_code: -4`:
  - execute path/controller path is failing.
- `ros2 action info /arm_controller/follow_joint_trajectory` -> `Action servers: 0`:
  - no trajectory controller is active.
- `ros2 control list_controllers` cannot contact service:
  - `controller_manager` is unavailable/crashed/not launched correctly.

### Required recovery target

- Ensure ros2_control bringup is healthy so these conditions are true at runtime:
  - `ros2 node list` includes `/controller_manager`.
  - `ros2 service list | rg "^/controller_manager/"` includes normal controller-manager APIs (`load/switch/unload/list`).
  - `ros2 control list_controllers` responds and shows:
    - `joint_state_broadcaster` as `active`
    - `arm_controller` as `active`
  - `ros2 action info /arm_controller/follow_joint_trajectory` shows `Action servers: 1`.

### Required config consistency

- MoveIt controller config and ros2_control controller config must match exactly:
  - same controller name (`arm_controller`)
  - same joint list (6 movable joints only)
  - action namespace set correctly (`follow_joint_trajectory`)
  - fixed joints excluded from trajectory controller joints

### Verification checklist after fix

- `GET /ready` reports both planning and execute backend readiness.
- RViz `Execute` succeeds.
- API `POST /execute` succeeds and updates robot state.
- `GET /current` and `GET /current_pose` reflect new post-execution pose.

### Known good behavior after resolution

- `/plan` computes and publishes visual trajectory to RViz.
- `/execute` runs the cached trajectory successfully.
- Cached plan is cleared only after successful execute (current policy).

* `host` could also be determined by running `hostname -I` in wsl. 

4. Add console entry point in `setup.py`.

- Place this block inside the `setup(...)` call in `src/moveit_solver_server/setup.py`.

```python
entry_points={
    'console_scripts': [
        'server = moveit_solver_server.server:main',
    ],
},
```

5. Rebuild package and verify executable.

```bash
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install --event-handlers console_direct+
source ./install/setup.bash
ros2 pkg executables moveit_solver_server
```

**Expected result**:
- `ros2 pkg executables moveit_solver_server` prints a line ending with `server`.
- Server code now exposes both `/health` and `/plan`.

**Possible problems**:
- `No executable found` due to missing/incorrect `entry_points`.
- `server.py` missing or `main()` missing.
- Build ran before file changes were saved.

---

## Step 5: Add external server interface (HTTP)

**Goal**: run server and validate HTTP behavior from Postman.

**Checklist**:

1. Start server from workspace root.

```bash
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
source ./install/setup.bash
source ./venv/bin/activate
ros2 run moveit_solver_server server
```

2. Verify health endpoint from a second terminal.

```bash
curl http://172.21.1.108:8000/health
```

Expected output:

```json
{"ok":true}
```

3. Verify plan endpoint from a second terminal.

```bash
curl -X POST http://172.21.1.108:8000/plan \
  -H 'Content-Type: application/json' \
  -d '{"frame_id":"world","target":[0.10,0.00,0.12,0.0,0.0,0.0,1.0],"current":[0.0,0.0,0.0,0.0,0.0,0.0]}'
```

Expected output contains:

```json
{"plan_success":true,"message":"Plan endpoint reached","frame_id":"world"}
```

4. In Postman, send `POST http://172.21.1.108:8000/plan` with `Content-Type: application/json`.

5. Use this request body (MoveIt canonical units):

```json
{
  "frame_id": "world",
  "target": [0.10, 0.00, 0.12, 0.0, 0.0, 0.0, 1.0],
  "current": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
}
```

6. Confirm response behavior.

**Expected result**:
- Server process stays alive after startup.
- `/health` works.
- `POST /plan` returns JSON (success/failure) and does not execute robot motion.

**Possible problems**:
- Port conflicts.
- `ModuleNotFoundError` from inactive `venv`.
- `404` from route mismatch.
- `No executable found` from stale or bad entry point.

**If you get `ModuleNotFoundError: No module named 'fastapi'`**:

This means `ros2 run` is using a Python interpreter that cannot see your `venv` site-packages.

1. Check which Python `ros2 run` wrapper uses.

```bash
head -n 1 "$WORKSPACE_ROOT/install/moveit_solver_server/lib/moveit_solver_server/server"
```

2. Recommended fix: bind this package to workspace `venv` by rebuilding with `venv` active.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
python -m pip install --upgrade pip
python -m pip install fastapi uvicorn pydantic
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
rm -rf build/moveit_solver_server install/moveit_solver_server
colcon build --packages-select moveit_solver_server --symlink-install --event-handlers console_direct+
source ./install/setup.bash
head -n 1 ./install/moveit_solver_server/lib/moveit_solver_server/server
ros2 run moveit_solver_server server
```

3. Alternative fallback: install for system/user Python used by ROS 2 and rebuild.

```bash
cd "$WORKSPACE_ROOT"
python3 -m pip install --user fastapi uvicorn pydantic
source /opt/ros/humble/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

---

## Step 6: Add validation and safety guards

**Goal**: reject malformed or unsafe requests before planning logic runs.

**Checklist**:

1. Update `src/moveit_solver_server/moveit_solver_server/server.py` with strict request validation.

```python
from fastapi import FastAPI
from pydantic import BaseModel, Field, field_validator, model_validator
import math
import uvicorn


class PlanRequest(BaseModel):
    frame_id: str
    target: list[float] = Field(min_length=7, max_length=7)
    current: list[float] = Field(min_length=6, max_length=6)

    @field_validator("frame_id")
    @classmethod
    def frame_id_not_empty(cls, value: str) -> str:
        if not value.strip():
            raise ValueError("frame_id must be a non-empty string")
        return value

    @field_validator("target", "current")
    @classmethod
    def all_numbers_finite(cls, values: list[float]) -> list[float]:
        for v in values:
            if not math.isfinite(v):
                raise ValueError("all numeric values must be finite")
        return values

    @model_validator(mode="after")
    def quaternion_is_normalized(self):
        qa, qb, qc, qd = self.target[3:7]
        q_norm = math.sqrt(qa * qa + qb * qb + qc * qc + qd * qd)
        if abs(q_norm - 1.0) > 1e-3:
            raise ValueError("target quaternion must be normalized (norm ~= 1.0)")
        return self


app = FastAPI()


@app.get("/health")
def health():
    return {"ok": True}


@app.post("/plan")
def plan(request: PlanRequest):
    return {
        "plan_success": True,
        "message": "Validation passed; planner integration next",
        "frame_id": request.frame_id,
    }


def main():
    uvicorn.run(app, host="172.21.1.108", port=8000)
```

2. Rebuild and run server.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

3. Validate error behavior from a second terminal.

Wrong `target` length:

```bash
curl -X POST http://172.21.1.108:8000/plan -H 'Content-Type: application/json' -d '{"frame_id":"world","target":[0,0,0,0,0,1],"current":[0,0,0,0,0,0]}'
```

Non-normalized quaternion:

```bash
curl -X POST http://172.21.1.108:8000/plan -H 'Content-Type: application/json' -d '{"frame_id":"world","target":[0.1,0.0,0.2,0,0,0,0],"current":[0,0,0,0,0,0]}'
```

4. Keep server planning-only (do not add execute endpoint in this step).

**Expected result**:
- Valid request returns HTTP `200`.
- Invalid request returns validation error response (HTTP `422`) with clear field-level details.
- Safety checks run before planner integration.

**Possible problems**:
- Invalid array lengths (`target` != 7, `current` != expected joint count).
- Non-finite numeric values (`NaN`, `inf`).
- Non-normalized quaternion.
- Generic errors without clear field messages.

---

## Step 7: Add status reporting and observability

**Goal**: add request-level traceability and clear server-side status logs for every `/plan` call.

**Checklist**:

1. Add request ID support and structured logging in `src/moveit_solver_server/moveit_solver_server/server.py`.

```python
from fastapi import FastAPI, Request
from pydantic import BaseModel, Field, field_validator, model_validator
import logging
import math
import time
import uuid
import uvicorn


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("moveit_solver_server")


class PlanRequest(BaseModel):
    frame_id: str
    target: list[float] = Field(min_length=7, max_length=7)
    current: list[float] = Field(min_length=6, max_length=6)

    @field_validator("frame_id")
    @classmethod
    def frame_id_not_empty(cls, value: str) -> str:
        if not value.strip():
            raise ValueError("frame_id must be a non-empty string")
        return value

    @field_validator("target", "current")
    @classmethod
    def all_numbers_finite(cls, values: list[float]) -> list[float]:
        for v in values:
            if not math.isfinite(v):
                raise ValueError("all numeric values must be finite")
        return values

    @model_validator(mode="after")
    def quaternion_is_normalized(self):
        qa, qb, qc, qd = self.target[3:7]
        q_norm = math.sqrt(qa * qa + qb * qb + qc * qc + qd * qd)
        if abs(q_norm - 1.0) > 1e-3:
            raise ValueError("target quaternion must be normalized (norm ~= 1.0)")
        return self


app = FastAPI()


@app.get("/health")
def health():
    return {"ok": True}


@app.post("/plan")
def plan(request: PlanRequest, http_request: Request):
    request_id = http_request.headers.get("x-request-id") or str(uuid.uuid4())
    started = time.time()
    logger.info("request_received request_id=%s frame_id=%s", request_id, request.frame_id)

    # Placeholder result in this step; real planner integration follows in next step.
    plan_success = True

    elapsed_ms = int((time.time() - started) * 1000)
    logger.info(
        "request_finished request_id=%s plan_success=%s duration_ms=%d",
        request_id,
        plan_success,
        elapsed_ms,
    )

    return {
        "request_id": request_id,
        "plan_success": plan_success,
        "message": "Validation passed; planner integration next",
        "duration_ms": elapsed_ms,
    }


def main():
    uvicorn.run(app, host="172.21.1.108", port=8000)
```

2. Rebuild and run.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

3. Test with explicit request ID.

```bash
curl -X POST http://172.21.1.108:8000/plan \
  -H 'Content-Type: application/json' \
  -H 'x-request-id: test-req-001' \
  -d '{"frame_id":"world","target":[0.10,0.00,0.12,0.0,0.0,0.0,1.0],"current":[0.0,0.0,0.0,0.0,0.0,0.0]}'
```

4. Test without `x-request-id` and confirm server generates one.

**Expected result**:
- Response contains `request_id`, `plan_success`, and `duration_ms`.
- Server logs include `request_received` and `request_finished` with the same request ID.
- Transport-level errors (HTTP/validation) and planning result are distinguishable.

**Possible problems**:
- Planner failure and transport failure not separated.
- Missing request correlation in logs.
- Request ID not returned in response.

---

## Step 8: Create launch flow and startup scripts

**Goal**: make startup repeatable with one command sequence for MoveIt + server.

**Checklist**:

1. Define terminal order and keep it consistent.

- Terminal A: launch MoveIt demo for your robot config.
- Terminal B: run HTTP solver server.
- Terminal C: send Postman/curl requests.

2. Terminal A commands (MoveIt):

```bash
export ROBOTNAME=my_robot_name
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT"
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
source ./install/setup.bash
ros2 launch ${ROBOTNAME}_config demo.launch.py
```

3. Terminal B commands (server):

```bash
export WORKSPACE_ROOT=~/repos/my_robot/ws_robot
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
source ./install/setup.bash
ros2 run moveit_solver_server server
```

4. Terminal C smoke tests:

```bash
curl http://172.21.1.108:8000/health
curl -X POST http://172.21.1.108:8000/plan -H 'Content-Type: application/json' -d '{"frame_id":"world","target":[0.10,0.00,0.12,0.0,0.0,0.0,1.0],"current":[0.0,0.0,0.0,0.0,0.0,0.0]}'
```

5. Optional helper aliases in current shell (for faster restarts).

- `ws_robot`: jumps to your workspace root and exports `WORKSPACE_ROOT`.
- `ws_source`: loads Python venv + ROS + MoveIt + local overlay for the current shell.
- Aliases are session-only by default; re-opened terminals will not have them unless added to `~/.bashrc`.

```bash
alias ws_robot='export WORKSPACE_ROOT=~/repos/my_robot/ws_robot && cd "$WORKSPACE_ROOT"'
alias ws_source='source ./venv/bin/activate && source /opt/ros/humble/setup.bash && source ~/ws_moveit2/install/setup.bash && source ./install/setup.bash'
```

Usage in a fresh terminal:

```bash
ws_robot; ws_source; ros2 run moveit_solver_server server
```

Usage to launch MoveIt quickly:

```bash
ws_robot; ws_source; ros2 launch ${ROBOTNAME}_config demo.launch.py
```

Optional: make aliases permanent.

```bash
echo "alias ws_robot='export WORKSPACE_ROOT=~/repos/my_robot/ws_robot && cd \"\$WORKSPACE_ROOT\"'" >> ~/.bashrc
echo "alias ws_source='source ./venv/bin/activate && source /opt/ros/humble/setup.bash && source ~/ws_moveit2/install/setup.bash && source ./install/setup.bash'" >> ~/.bashrc
source ~/.bashrc
```

If an alias is not found in a new shell:

```bash
source ~/.bashrc
```

**Expected result**:
- MoveIt launches in RViz from Terminal A.
- Server starts on `172.21.1.108:8000` from Terminal B.
- Terminal C gets valid JSON from `/health` and `/plan`.
- Restart process is predictable and repeatable.

**Possible problems**:
- Startup race conditions.
- Unsourced overlay at runtime.
- MoveIt launched in one workspace but server started from another.

---

## Step 9: Run end-to-end tests with a simple client

**Goal**: verify end-to-end request flow from Postman against the live server.

**Checklist**:

1. Keep MoveIt and server running (from Step 8) in separate terminals.

2. Postman happy-path test.

- Method: `POST`
- URL: `http://172.21.1.108:8000/plan`
- Header: `Content-Type: application/json`
- Header: `x-request-id: postman-happy-001`
- Body:

```json
{
  "frame_id": "world",
  "target": [0.10, 0.00, 0.12, 0.0, 0.0, 0.0, 1.0],
  "current": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
}
```

3. Postman failure test: invalid quaternion.

- Keep same URL/headers.
- Body:

```json
{
  "frame_id": "world",
  "target": [0.10, 0.00, 0.12, 0.0, 0.0, 0.0, 0.0],
  "current": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
}
```

4. Postman failure test: invalid array length.

- Keep same URL/headers.
- Body:

```json
{
  "frame_id": "world",
  "target": [0.10, 0.00, 0.12, 0.0, 0.0, 0.0, 1.0],
  "current": [0.0, 0.0, 0.0]
}
```

5. Confirm server logs contain request IDs and final status lines for each request.

**Expected result**:
- Happy-path request returns HTTP `200` with JSON containing `request_id` and `plan_success`.
- Invalid requests return HTTP `422` with clear validation details.
- Server stays alive after both success and failure requests.
- No motion execute endpoint is invoked.

**Possible problems**:
- Planner succeeds but request parsing fails.
- Duplicate request retries without idempotency handling.
- Postman sends malformed JSON (trailing commas/quotes).
- Missing `Content-Type: application/json` header.

---

## Step 10: Harden for real usage

```
**Status**: skipped for now by decision.

**Current scope**:
- No authentication in this phase.
- Keep planning-only behavior and existing validation/observability.
- Revisit hardening when moving beyond local/dev usage.
```

**Goal**: protect the server from unsafe clients and make failure handling predictable.

**Checklist**:

1. Add API key authentication for `/plan`.

- Require header: `x-api-key: <SERVER_API_KEY>`.
- If key missing/invalid, return HTTP `401`.
- Load key from environment variable: `SERVER_API_KEY`.

2. Add basic rate limiting.

- Start with per-client limit: `30 requests/min`.
- Return HTTP `429` when exceeded.
- Include `Retry-After` header in throttled responses.

3. Add server-side planning timeout guard.

- Keep timeout at `30` seconds.
- If planning exceeds timeout, return HTTP `504` with a clear timeout error.

4. Add deterministic error contract.

- Always return JSON error body with these fields:
  - `request_id`
  - `error_code`
  - `message`
- Keep transport errors distinct from planner failures.

5. Add basic watchdog/health checks.

- `/health` should remain lightweight and fast.
- Add `/ready` endpoint that confirms planner dependencies are available.
- Return HTTP `503` from `/ready` when dependencies are not ready.

6. Add startup and restart policy.

- Start policy: MoveIt first, then server.
- On server crash: restart server process and validate `/health` before accepting requests.
- On repeated failures: stop accepting `/plan` and surface clear operator message.

7. Add security hygiene for logs and configuration.

- Never log API keys, tokens, or full sensitive headers.
- Log request IDs and status only.
- Keep `SERVER_API_KEY` out of source files and Git.

8. Verify hardening behavior with manual tests.

- Missing API key -> expect `401`.
- Wrong API key -> expect `401`.
- Request burst above limit -> expect `429`.
- Artificially delayed planning over timeout -> expect `504`.
- Ready check before/after MoveIt startup -> expect `503` then `200`.

**Expected result**:
- Unauthorized callers cannot use `/plan`.
- Request spikes are throttled without crashing the server.
- Timeouts and failures return deterministic JSON errors.
- Operators can determine service state via `/health` and `/ready`.

**Possible problems**:
- Unauthorized command source.
- Unbounded request rate.
- No recovery sequence after emergency stop.
- API key set in shell but not visible to launched process.
- Rate limiting that incorrectly blocks all clients equally.

---

## Step 11: Wire `/plan` to MoveIt planning backend

**Status**: active.

**Goal**: connect the server process to MoveIt services/actions and block `/plan` when MoveIt is unavailable.

**Checklist**:

1. Update `src/moveit_solver_server/moveit_solver_server/server.py` to add a MoveIt bridge object.

```python
from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel, Field, field_validator, model_validator
import logging
import math
import time
import uuid
import uvicorn

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from moveit_msgs.action import MoveGroup


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("moveit_solver_server")


class MoveItBridge:
    def __init__(self):
        if not rclpy.ok():
            rclpy.init(args=None)
        self.node = Node("moveit_solver_server_node")
        self.move_group_client = ActionClient(self.node, MoveGroup, "/move_action")
        self.ready = False

    def connect(self, timeout_sec: float = 3.0) -> bool:
        self.ready = self.move_group_client.wait_for_server(timeout_sec=timeout_sec)
        return self.ready


class PlanRequest(BaseModel):
    frame_id: str
    target: list[float] = Field(min_length=7, max_length=7)
    current: list[float] = Field(min_length=6, max_length=6)

    @field_validator("frame_id")
    @classmethod
    def frame_id_not_empty(cls, value: str) -> str:
        if not value.strip():
            raise ValueError("frame_id must be a non-empty string")
        return value

    @field_validator("target", "current")
    @classmethod
    def all_numbers_finite(cls, values: list[float]) -> list[float]:
        for v in values:
            if not math.isfinite(v):
                raise ValueError("all numeric values must be finite")
        return values

    @model_validator(mode="after")
    def quaternion_is_normalized(self):
        qa, qb, qc, qd = self.target[3:7]
        q_norm = math.sqrt(qa * qa + qb * qb + qc * qc + qd * qd)
        if abs(q_norm - 1.0) > 1e-3:
            raise ValueError("target quaternion must be normalized (norm ~= 1.0)")
        return self


app = FastAPI()
bridge = MoveItBridge()


@app.on_event("startup")
def startup_event():
    ready = bridge.connect(timeout_sec=3.0)
    logger.info("moveit_bridge_ready=%s", ready)


@app.get("/health")
def health():
    return {"ok": True}


@app.get("/ready")
def ready():
    return {"moveit_ready": bridge.ready}


@app.post("/plan")
def plan(request: PlanRequest, http_request: Request):
    request_id = http_request.headers.get("x-request-id") or str(uuid.uuid4())
    started = time.time()
    logger.info("request_received request_id=%s frame_id=%s", request_id, request.frame_id)

    if not bridge.ready:
        raise HTTPException(status_code=503, detail="MoveIt backend not ready")

    # Step 11 wiring only: backend connected check complete.
    # Real goal mapping + planner call will be added in Step 12/13.
    elapsed_ms = int((time.time() - started) * 1000)
    logger.info("request_finished request_id=%s plan_success=%s duration_ms=%d", request_id, True, elapsed_ms)
    return {
        "request_id": request_id,
        "plan_success": True,
        "message": "MoveIt backend connected; real planning mapping in next step",
        "duration_ms": elapsed_ms,
    }


def main():
    uvicorn.run(app, host="172.21.1.108", port=8000)
```

2. Rebuild and run server.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

3. Verify readiness in a separate terminal.

```bash
curl http://172.21.1.108:8000/ready
```

Expected output when MoveIt demo is running:

```json
{"moveit_ready":true}
```

4. Verify dependency gating.

- Stop MoveIt launch process.
- Call `POST /plan` again.
- Expect HTTP `503` with message `MoveIt backend not ready`.

5. Restart MoveIt and restart server, then re-check `/ready`.

**Expected result**:
- `/ready` reports real MoveIt action-server connectivity.
- `/plan` is blocked when MoveIt is unavailable.
- Server is now connected to MoveIt runtime, ready for Step 12 goal mapping.

---

## Step 12: Map request payload to real planning targets

**Status**: active.

**Goal**: convert incoming JSON into concrete planning targets for MoveIt.

**Recommended code organization (use this instead of one large `server.py`)**:

- `src/moveit_solver_server/moveit_solver_server/server.py`
  - FastAPI app setup and route handlers only (`/health`, `/ready`, `/plan`).
- `src/moveit_solver_server/moveit_solver_server/models.py`
  - `PlanRequest` and validation rules.
- `src/moveit_solver_server/moveit_solver_server/bridge.py`
  - `MoveItBridge` class and action client lifecycle.
- `src/moveit_solver_server/moveit_solver_server/mapping.py`
  - Payload-to-MoveIt mapping helpers (`build_start_state`, `build_pose_target`, `build_goal_constraints`, `build_move_group_goal`).
- `src/moveit_solver_server/moveit_solver_server/constants.py`
  - `PLANNING_GROUP`, `PLANNING_TIMEOUT_SEC`, `JOINT_ORDER`.
- `src/moveit_solver_server/moveit_solver_server/moveit_handler.py`
  - Self-contained MoveIt service helpers for current state (via `/get_planning_scene`).

Note:
- The snippets in Steps 11-13 are shown inline for clarity, but you should place functions/classes in the files above for readability.
- `server.py` should import from these modules instead of containing all logic directly.

**Checklist**:

1. Define server-side planning constants in `server.py`.

- If using modular layout, place this in `constants.py` and import into `server.py` and `mapping.py`.

```python
PLANNING_GROUP = "arm"
PLANNING_TIMEOUT_SEC = 30.0

# Replace with your real joint order from MoveIt config.
JOINT_ORDER = [
    "base.stp_base_to_shoulder",
    "shoulder.stp_shoulder_to_upper_arm",
    "upper_arm.stp_upper_arm_to_lower_arm",
    "lower_arm.stp_lower_arm_to_slider",
    "slider.stp_slider_to_upper_wrist",
    "upper_wrist.stp_upper_wrist_to_lower_wrist",
]
```

What this means:
- `PLANNING_GROUP` must exactly match the MoveIt planning group name (example: `arm`).
- `PLANNING_TIMEOUT_SEC` is the max planner budget per request and should stay aligned with your Step 1 decision (`30` seconds).
- `JOINT_ORDER` is the exact mapping between `current` array indices and robot joint names; names and order must match your MoveIt config exactly.
- Do not include fixed joints in `JOINT_ORDER` (for your robot, exclude `lower_wrist.stp_lower_wrist_to_spray_cone`).
- For your current SRDF/URDF setup, `current` should contain 6 values matching the 6 movable joints in `JOINT_ORDER`.
- If `JOINT_ORDER` is wrong, values are assigned to wrong joints and planning results become invalid/unreliable.
- Place this constants block in `src/moveit_solver_server/moveit_solver_server/server.py` after imports/logger setup and before class/function definitions so all handlers/helpers can use it.

2. Add mapping helpers to `server.py` (request -> MoveIt messages).

- If using modular layout, place these functions in `mapping.py` and import them into `server.py`.

```python
from geometry_msgs.msg import PoseStamped
from moveit_msgs.msg import Constraints, JointConstraint, PositionConstraint, OrientationConstraint, BoundingVolume
from shape_msgs.msg import SolidPrimitive
from sensor_msgs.msg import JointState


def build_start_state(current: list[float]) -> JointState:
    js = JointState()
    js.name = JOINT_ORDER
    js.position = current
    return js


def build_pose_target(frame_id: str, target: list[float]) -> PoseStamped:
    x, y, z, qa, qb, qc, qd = target
    pose = PoseStamped()
    pose.header.frame_id = frame_id
    pose.pose.position.x = x
    pose.pose.position.y = y
    pose.pose.position.z = z
    pose.pose.orientation.x = qa
    pose.pose.orientation.y = qb
    pose.pose.orientation.z = qc
    pose.pose.orientation.w = qd
    return pose


def build_goal_constraints(frame_id: str, target: list[float]) -> Constraints:
    x, y, z, qa, qb, qc, qd = target

    c = Constraints()

    pos = PositionConstraint()
    pos.header.frame_id = frame_id
    pos.constraint_region.primitives = [SolidPrimitive(type=SolidPrimitive.SPHERE, dimensions=[0.005])]
    pos.constraint_region.primitive_poses = [build_pose_target(frame_id, target).pose]
    pos.weight = 1.0

    ori = OrientationConstraint()
    ori.header.frame_id = frame_id
    ori.orientation.x = qa
    ori.orientation.y = qb
    ori.orientation.z = qc
    ori.orientation.w = qd
    ori.absolute_x_axis_tolerance = 0.01
    ori.absolute_y_axis_tolerance = 0.01
    ori.absolute_z_axis_tolerance = 0.01
    ori.weight = 1.0

    c.position_constraints = [pos]
    c.orientation_constraints = [ori]
    return c
```

3. In `/plan`, call these helpers and log mapped values before planner call.

```python
start_state = build_start_state(request.current)
pose_target = build_pose_target(request.frame_id, request.target)
goal_constraints = build_goal_constraints(request.frame_id, request.target)

logger.info(
    "request_mapped request_id=%s frame_id=%s planning_group=%s",
    request_id,
    request.frame_id,
    PLANNING_GROUP,
)
```

4. Validate joint order and frame before planner integration.

- Verify `JOINT_ORDER` exactly matches your MoveIt controller/planning group order.
- Verify `frame_id` exists in the planning scene (example: `world`).
- Keep unit assumptions fixed (radians/meters).

5. Rebuild and run after mapping changes.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

**Expected result**:
- `/plan` logs show request mapping completed with correct `frame_id` and planning group.
- Start state and target pose are built from request payload without unit conversion.
- Server is ready for Step 13 real planner call/response wiring.

6. Create `src/moveit_solver_server/moveit_solver_server/moveit_handler.py` for current-state access.

- Use `GetPlanningScene` service (`components=ROBOT_STATE`) as the source of current joints.
- Do not depend on `/joint_states` publisher for this endpoint.
- Expose one method like `get_current_in_order(JOINT_ORDER, timeout_sec=2.0)` that returns ordered joint values.
- Raise clear exceptions for service unavailable, timeout/failure, or missing joints.

---

## Step 13: Return real planning output to client

**Status**: active.

**Goal**: make `/plan` and `/current` return real MoveIt-backed data.

**Checklist**:

1. Keep `/plan` action-based and return deterministic planner output.

- Keep `build_move_group_goal(...)` in `mapping.py`.
- Keep request/response logging with `request_id`.
- Keep explicit handling for `GOAL_REJECTED` and `PLANNING_TIMEOUT`.
- Ensure goal constraints specify the constrained link (`EE_LINK`), otherwise planning often fails.

Copy/paste reference for `constants.py`:

```python
PLANNING_GROUP = "arm"
PLANNING_TIMEOUT_SEC = 30.0
EXECUTION_TIMEOUT_SEC = 30.0
EE_LINK = "spray_cone.stp"

JOINT_ORDER = [
    "base.stp_base_to_shoulder",
    "shoulder.stp_shoulder_to_upper_arm",
    "upper_arm.stp_upper_arm_to_lower_arm",
    "lower_arm.stp_lower_arm_to_slider",
    "slider.stp_slider_to_upper_wrist",
    "upper_wrist.stp_upper_wrist_to_lower_wrist",
]
```

Copy/paste reference for `mapping.py`:

```python
from geometry_msgs.msg import PoseStamped
from moveit_msgs.action import MoveGroup
from moveit_msgs.msg import Constraints, MotionPlanRequest, OrientationConstraint, PositionConstraint
from sensor_msgs.msg import JointState
from shape_msgs.msg import SolidPrimitive

from .constants import EE_LINK, JOINT_ORDER, PLANNING_GROUP, PLANNING_TIMEOUT_SEC
from .models import PlanRequest


def build_start_state(current: list[float]) -> JointState:
    js = JointState()
    js.name = JOINT_ORDER
    js.position = current
    return js


def build_pose_target(frame_id: str, target: list[float]) -> PoseStamped:
    x, y, z, qx, qy, qz, qw = target
    pose = PoseStamped()
    pose.header.frame_id = frame_id
    pose.pose.position.x = x
    pose.pose.position.y = y
    pose.pose.position.z = z
    pose.pose.orientation.x = qx
    pose.pose.orientation.y = qy
    pose.pose.orientation.z = qz
    pose.pose.orientation.w = qw
    return pose


def build_goal_constraints(frame_id: str, target: list[float]) -> Constraints:
    x, y, z, qx, qy, qz, qw = target

    c = Constraints()

    pos = PositionConstraint()
    pos.header.frame_id = frame_id
    pos.link_name = EE_LINK
    pos.constraint_region.primitives = [
        SolidPrimitive(type=SolidPrimitive.SPHERE, dimensions=[0.005])
    ]
    pos.constraint_region.primitive_poses = [build_pose_target(frame_id, target).pose]
    pos.weight = 1.0

    ori = OrientationConstraint()
    ori.header.frame_id = frame_id
    ori.link_name = EE_LINK
    ori.orientation.x = qx
    ori.orientation.y = qy
    ori.orientation.z = qz
    ori.orientation.w = qw
    ori.absolute_x_axis_tolerance = 0.01
    ori.absolute_y_axis_tolerance = 0.01
    ori.absolute_z_axis_tolerance = 0.01
    ori.weight = 1.0

    c.position_constraints = [pos]
    c.orientation_constraints = [ori]
    return c


def build_move_group_goal(request: PlanRequest) -> MoveGroup.Goal:
    start_state = build_start_state(request.current)
    goal_constraints = build_goal_constraints(request.frame_id, request.target)

    motion_request = MotionPlanRequest()
    motion_request.group_name = PLANNING_GROUP
    motion_request.allowed_planning_time = PLANNING_TIMEOUT_SEC
    motion_request.start_state.joint_state = start_state
    motion_request.goal_constraints = [goal_constraints]

    goal = MoveGroup.Goal()
    goal.request = motion_request
    goal.planning_options.plan_only = True
    return goal
```

Copy/paste reference for `/plan` in `server.py`:

```python
@app.post("/plan")
def plan(request: PlanRequest, http_request: Request):
    request_id = http_request.headers.get("x-request-id") or str(uuid.uuid4())
    started = time.time()
    logger.info("request_received request_id=%s frame_id=%s", request_id, request.frame_id)

    # Re-check readiness on each request so late MoveIt startup can recover.
    if not bridge.connect(timeout_sec=0.5):
        raise HTTPException(status_code=503, detail="MoveIt backend not ready")

    try:
        goal_msg = build_move_group_goal(request)

        send_future = bridge.move_group_client.send_goal_async(goal_msg)
        rclpy.spin_until_future_complete(bridge.node, send_future, timeout_sec=PLANNING_TIMEOUT_SEC)
        goal_handle = send_future.result()

        if goal_handle is None or not goal_handle.accepted:
            elapsed_ms = int((time.time() - started) * 1000)
            return {
                "request_id": request_id,
                "plan_success": False,
                "error_code": "GOAL_REJECTED",
                "message": "MoveIt rejected planning goal",
                "duration_ms": elapsed_ms,
            }

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(bridge.node, result_future, timeout_sec=PLANNING_TIMEOUT_SEC)
        result_wrap = result_future.result()

        if result_wrap is None:
            elapsed_ms = int((time.time() - started) * 1000)
            return {
                "request_id": request_id,
                "plan_success": False,
                "error_code": "PLANNING_TIMEOUT",
                "message": "Planning did not return before timeout",
                "duration_ms": elapsed_ms,
            }

        result = result_wrap.result
        error_val = int(result.error_code.val)
        plan_success = (error_val == 1)
        traj_points = len(result.planned_trajectory.joint_trajectory.points)
        planning_time_sec = float(result.planning_time)
        elapsed_ms = int((time.time() - started) * 1000)

        response = {
            "request_id": request_id,
            "plan_success": plan_success,
            "error_code": error_val,
            "message": "Planning success" if plan_success else "Planning failed",
            "duration_ms": elapsed_ms,
            "planning_time_sec": planning_time_sec,
            "trajectory_points": traj_points,
        }

        logger.info(
            "request_finished request_id=%s plan_success=%s error_code=%d duration_ms=%d points=%d",
            request_id,
            plan_success,
            error_val,
            elapsed_ms,
            traj_points,
        )
        return response

    except HTTPException:
        raise
    except Exception as exc:
        elapsed_ms = int((time.time() - started) * 1000)
        logger.exception("request_failed request_id=%s duration_ms=%d", request_id, elapsed_ms)
        raise HTTPException(status_code=500, detail=f"Internal planning bridge error: {exc}")
```

2. Use `moveit_handler.py` for `/current` endpoint.

- Import in `server.py`:

```python
from .moveit_handler import (
    MoveItHandler,
    MoveItHandlerError,
)
```

- Initialize once at startup-level:

```python
moveit_handler = MoveItHandler()
```

- Add `/current` endpoint:

```python
@app.get("/current")
def current_state():
    try:
        result = moveit_handler.get_current_in_order(JOINT_ORDER, timeout_sec=2.0)
        return {
            "joint_names": result.joint_names,
            "current": result.current,
            "source": result.source,
            "units": result.units,
        }
    except MoveItHandlerError as exc:
        raise HTTPException(status_code=503, detail=str(exc))
```

2.1 Add `/current_pose` endpoint (FK-based Cartesian pose).

- Use MoveIt FK service (`/compute_fk`) so Cartesian pose comes from the same planning stack.
- Default tip link: `spray_cone.stp`.
- Default frame: `world`.

Add imports to `moveit_handler.py`:

```python
from moveit_msgs.srv import GetPositionFK
```

Add a result model in `moveit_handler.py`:

```python
@dataclass
class CurrentPoseResult:
    frame_id: str
    link_name: str
    position: dict
    orientation: dict
    source: str = "moveit/compute_fk"
```

Create FK client in `MoveItHandler.__init__`:

```python
self.compute_fk_client = self.node.create_client(GetPositionFK, "/compute_fk")
```

Add method in `moveit_handler.py`:

```python
def get_current_pose(
    self,
    joint_order: Sequence[str],
    link_name: str = "spray_cone.stp",
    frame_id: str = "world",
    timeout_sec: float = 2.0,
) -> CurrentPoseResult:
    # 1) Get current joint state from planning scene
    joint_state = self._fetch_joint_state(timeout_sec=timeout_sec)

    # 2) Ensure required joints exist
    index_map = {name: i for i, name in enumerate(joint_state.name)}
    missing = [j for j in joint_order if j not in index_map]
    if missing:
        raise MoveItMissingJoints(f"Missing joints in MoveIt robot_state: {missing}")

    # 3) Build ordered RobotState for FK request
    ordered_names = list(joint_order)
    ordered_positions = [float(joint_state.position[index_map[j]]) for j in ordered_names]

    # 4) Call FK service
    if not self.compute_fk_client.wait_for_service(timeout_sec=timeout_sec):
        raise MoveItServiceUnavailable("compute_fk service not available")

    req = GetPositionFK.Request()
    req.header.frame_id = frame_id
    req.fk_link_names = [link_name]
    req.robot_state.joint_state.name = ordered_names
    req.robot_state.joint_state.position = ordered_positions

    future = self.compute_fk_client.call_async(req)
    rclpy.spin_until_future_complete(self.node, future, timeout_sec=timeout_sec)
    resp = future.result()
    if resp is None:
        raise MoveItServiceCallFailed("compute_fk call timed out or failed")

    if not resp.pose_stamped:
        code = int(resp.error_code.val)
        raise MoveItServiceCallFailed(f"compute_fk returned no pose (error_code={code})")

    pose = resp.pose_stamped[0].pose
    return CurrentPoseResult(
        frame_id=frame_id,
        link_name=link_name,
        position={"x": pose.position.x, "y": pose.position.y, "z": pose.position.z},
        orientation={
            "x": pose.orientation.x,
            "y": pose.orientation.y,
            "z": pose.orientation.z,
            "w": pose.orientation.w,
        },
    )
```

Add `/current_pose` route in `server.py`:

```python
@app.get("/current_pose")
def current_pose(
    link_name: str = "spray_cone.stp",
    frame_id: str = "world",
):
    try:
        result = moveit_handler.get_current_pose(
            JOINT_ORDER,
            link_name=link_name,
            frame_id=frame_id,
            timeout_sec=2.0,
        )
        return {
            "frame_id": result.frame_id,
            "link_name": result.link_name,
            "position": result.position,
            "orientation": result.orientation,
            "source": result.source,
        }
    except MoveItHandlerError as exc:
        raise HTTPException(status_code=503, detail=str(exc))
```

Validate service availability and endpoint:

```bash
ros2 service list | rg compute_fk
curl "http://172.21.1.108:8000/current_pose"
curl "http://172.21.1.108:8000/current_pose?link_name=spray_cone.stp&frame_id=world"
```

3. Add shutdown cleanup.

```python
@app.on_event("shutdown")
def shutdown_event():
    moveit_handler.close()
```

4. Rebuild and run.

```bash
cd "$WORKSPACE_ROOT"
source ./venv/bin/activate
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
colcon build --packages-select moveit_solver_server --symlink-install
source ./install/setup.bash
ros2 run moveit_solver_server server
```

5. Validate all endpoints.

- `GET /health` -> basic liveness.
- `GET /ready` -> MoveIt backend readiness.
- `GET /current` -> current joint values from MoveIt planning scene.
- `GET /current_pose` -> current Cartesian pose from MoveIt FK service.
- `POST /plan` -> real planning success/failure.

**Expected result**:
- Response now comes from real MoveIt planner output, not stub text.
- Client can distinguish validation failure (`422`), backend unavailable (`503`), timeout, and planner failure.
- Successful plans report non-zero `trajectory_points` and planning time.
- `/current` returns current robot joint values in deterministic `JOINT_ORDER` from `get_planning_scene`.
- `/current_pose` returns tip-link Cartesian pose in requested frame.

**Common integration pitfall**:
- Startup error `AttributeError: 'MoveItBridge' object has no attribute 'connect'` means `connect(...)` was dropped while editing `MoveItBridge`; re-add it, rebuild package, and re-source workspace.
- `GET /current` returning `No joint state received yet` indicates old `/joint_states` subscriber-based code is still active; switch to `moveit_handler.py` service-based implementation.
- `GET /current_pose` failure with compute_fk unavailable means MoveIt is not fully up/sourced; verify `/compute_fk` exists before calling endpoint.

---

## Step 14: Verify real planner integration end-to-end

**Status**: placeholder.

**Goal**: prove the server is truly connected to MoveIt and not returning stub output.

**Checklist**:
- Happy-path request returns planner-backed success/failure.
- Invalid frame request returns clear planner/frame error.
- MoveIt stopped -> `/plan` fails with clear dependency/service-unavailable message.
- MoveIt restarted -> `/plan` recovers without code changes.
- `/current` returns ordered values while MoveIt is up.
- MoveIt stopped -> `/current` returns `503` with clear service-unavailable message.

**Expected result**:
- Server and MoveIt are functionally connected through `/plan`.

---

## Appendix

### `constants.py`

```python
PLANNING_GROUP = "arm"
PLANNING_TIMEOUT_SEC = 30.0
EE_LINK = "spray_cone.stp"

JOINT_ORDER = [
    "base.stp_base_to_shoulder",
    "shoulder.stp_shoulder_to_upper_arm",
    "upper_arm.stp_upper_arm_to_lower_arm",
    "lower_arm.stp_lower_arm_to_slider",
    "slider.stp_slider_to_upper_wrist",
    "upper_wrist.stp_upper_wrist_to_lower_wrist",
]
```

### `models.py`

```python
import math

from pydantic import BaseModel, Field, field_validator, model_validator


class PlanRequest(BaseModel):
    frame_id: str
    target: list[float] = Field(min_length=7, max_length=7)
    current: list[float] = Field(min_length=6, max_length=6)

    @field_validator("frame_id")
    @classmethod
    def frame_id_not_empty(cls, value: str) -> str:
        if not value.strip():
            raise ValueError("frame_id must be a non-empty string")
        return value

    @field_validator("target", "current")
    @classmethod
    def all_numbers_finite(cls, values: list[float]) -> list[float]:
        for v in values:
            if not math.isfinite(v):
                raise ValueError("all numeric values must be finite")
        return values

    @model_validator(mode="after")
    def quaternion_is_normalized(self):
        qx, qy, qz, qw = self.target[3:7]
        q_norm = math.sqrt(qx * qx + qy * qy + qz * qz + qw * qw)
        if abs(q_norm - 1.0) > 1e-3:
            raise ValueError("target quaternion must be normalized (norm ~= 1.0)")
        return self
```

### `mapping.py`

```python
from geometry_msgs.msg import PoseStamped
from moveit_msgs.action import MoveGroup
from moveit_msgs.msg import (
    Constraints,
    MotionPlanRequest,
    OrientationConstraint,
    PositionConstraint,
)
from sensor_msgs.msg import JointState
from shape_msgs.msg import SolidPrimitive

from .constants import EE_LINK, JOINT_ORDER, PLANNING_GROUP, PLANNING_TIMEOUT_SEC
from .models import PlanRequest


def build_start_state(current: list[float]) -> JointState:
    js = JointState()
    js.name = JOINT_ORDER
    js.position = current
    return js


def build_pose_target(frame_id: str, target: list[float]) -> PoseStamped:
    x, y, z, qx, qy, qz, qw = target
    pose = PoseStamped()
    pose.header.frame_id = frame_id
    pose.pose.position.x = x
    pose.pose.position.y = y
    pose.pose.position.z = z
    pose.pose.orientation.x = qx
    pose.pose.orientation.y = qy
    pose.pose.orientation.z = qz
    pose.pose.orientation.w = qw
    return pose


def build_goal_constraints(frame_id: str, target: list[float]) -> Constraints:
    qx, qy, qz, qw = target[3:7]

    c = Constraints()

    pos = PositionConstraint()
    pos.header.frame_id = frame_id
    pos.link_name = EE_LINK
    pos.constraint_region.primitives = [
        SolidPrimitive(type=SolidPrimitive.SPHERE, dimensions=[0.005])
    ]
    pos.constraint_region.primitive_poses = [build_pose_target(frame_id, target).pose]
    pos.weight = 1.0

    ori = OrientationConstraint()
    ori.header.frame_id = frame_id
    ori.link_name = EE_LINK
    ori.orientation.x = qx
    ori.orientation.y = qy
    ori.orientation.z = qz
    ori.orientation.w = qw
    ori.absolute_x_axis_tolerance = 0.01
    ori.absolute_y_axis_tolerance = 0.01
    ori.absolute_z_axis_tolerance = 0.01
    ori.weight = 1.0

    c.position_constraints = [pos]
    c.orientation_constraints = [ori]
    return c


def build_move_group_goal(request: PlanRequest) -> MoveGroup.Goal:
    start_state = build_start_state(request.current)
    goal_constraints = build_goal_constraints(request.frame_id, request.target)

    motion_request = MotionPlanRequest()
    motion_request.group_name = PLANNING_GROUP
    motion_request.allowed_planning_time = PLANNING_TIMEOUT_SEC
    motion_request.start_state.joint_state = start_state
    motion_request.goal_constraints = [goal_constraints]

    goal = MoveGroup.Goal()
    goal.request = motion_request
    goal.planning_options.plan_only = True
    return goal
```

### `moveit_handler.py`

```python
from dataclasses import dataclass
from typing import Sequence

import rclpy
from rclpy.node import Node
from moveit_msgs.msg import PlanningSceneComponents
from moveit_msgs.srv import GetPlanningScene, GetPositionFK


class MoveItHandlerError(Exception):
    pass


class MoveItServiceUnavailable(MoveItHandlerError):
    pass


class MoveItServiceCallFailed(MoveItHandlerError):
    pass


class MoveItMissingJoints(MoveItHandlerError):
    pass


@dataclass
class CurrentStateResult:
    joint_names: list[str]
    current: list[float]
    source: str = "moveit/get_planning_scene"
    units: str = "rad_or_m_by_joint_type"


@dataclass
class CurrentPoseResult:
    frame_id: str
    link_name: str
    position: dict
    orientation: dict
    source: str = "moveit/compute_fk"


class MoveItHandler:
    def __init__(self, node_name: str = "moveit_handler_node") -> None:
        self._owns_context = False
        if not rclpy.ok():
            rclpy.init(args=None)
            self._owns_context = True

        self.node = Node(node_name)
        self.get_planning_scene_client = self.node.create_client(
            GetPlanningScene, "/get_planning_scene"
        )
        self.compute_fk_client = self.node.create_client(GetPositionFK, "/compute_fk")

    def _fetch_joint_state(self, timeout_sec: float = 2.0):
        if not self.get_planning_scene_client.wait_for_service(timeout_sec=timeout_sec):
            raise MoveItServiceUnavailable("get_planning_scene service not available")

        req = GetPlanningScene.Request()
        req.components.components = PlanningSceneComponents.ROBOT_STATE

        future = self.get_planning_scene_client.call_async(req)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=timeout_sec)
        resp = future.result()
        if resp is None:
            raise MoveItServiceCallFailed("get_planning_scene call timed out or failed")

        return resp.scene.robot_state.joint_state

    def get_current_in_order(
        self,
        joint_order: Sequence[str],
        timeout_sec: float = 2.0,
    ) -> CurrentStateResult:
        js = self._fetch_joint_state(timeout_sec=timeout_sec)
        index_map = {name: i for i, name in enumerate(js.name)}
        missing = [j for j in joint_order if j not in index_map]
        if missing:
            raise MoveItMissingJoints(f"Missing joints in MoveIt robot_state: {missing}")

        ordered = [float(js.position[index_map[j]]) for j in joint_order]
        return CurrentStateResult(joint_names=list(joint_order), current=ordered)

    def get_current_pose(
        self,
        joint_order: Sequence[str],
        link_name: str = "spray_cone.stp",
        frame_id: str = "world",
        timeout_sec: float = 2.0,
    ) -> CurrentPoseResult:
        js = self._fetch_joint_state(timeout_sec=timeout_sec)

        index_map = {name: i for i, name in enumerate(js.name)}
        missing = [j for j in joint_order if j not in index_map]
        if missing:
            raise MoveItMissingJoints(f"Missing joints in MoveIt robot_state: {missing}")

        if not self.compute_fk_client.wait_for_service(timeout_sec=timeout_sec):
            raise MoveItServiceUnavailable("compute_fk service not available")

        ordered_names = list(joint_order)
        ordered_positions = [float(js.position[index_map[j]]) for j in ordered_names]

        req = GetPositionFK.Request()
        req.header.frame_id = frame_id
        req.fk_link_names = [link_name]
        req.robot_state.joint_state.name = ordered_names
        req.robot_state.joint_state.position = ordered_positions

        future = self.compute_fk_client.call_async(req)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=timeout_sec)
        resp = future.result()
        if resp is None:
            raise MoveItServiceCallFailed("compute_fk call timed out or failed")

        if not resp.pose_stamped:
            code = int(resp.error_code.val)
            raise MoveItServiceCallFailed(f"compute_fk returned no pose (error_code={code})")

        pose = resp.pose_stamped[0].pose
        return CurrentPoseResult(
            frame_id=frame_id,
            link_name=link_name,
            position={"x": pose.position.x, "y": pose.position.y, "z": pose.position.z},
            orientation={
                "x": pose.orientation.x,
                "y": pose.orientation.y,
                "z": pose.orientation.z,
                "w": pose.orientation.w,
            },
        )

    def close(self) -> None:
        try:
            self.node.destroy_node()
        finally:
            if self._owns_context and rclpy.ok():
                rclpy.shutdown()
```

### `execution.py`

```python
from threading import Lock
import time

import rclpy
from moveit_msgs.action import ExecuteTrajectory


class ExecuteError(Exception):
    pass


class NoCachedPlanError(ExecuteError):
    pass


class ExecuteRejectedError(ExecuteError):
    pass


class ExecuteTimeoutError(ExecuteError):
    pass


class ExecutionManager:
    def __init__(self) -> None:
        self._lock = Lock()
        self._last_plan = None
        self._last_start_state = None
        self._last_plan_request_id = None

    def store_plan(self, request_id, trajectory_start, planned_trajectory) -> None:
        with self._lock:
            self._last_plan_request_id = request_id
            self._last_start_state = trajectory_start
            self._last_plan = planned_trajectory

    def has_plan(self) -> bool:
        with self._lock:
            return self._last_plan is not None

    def clear_plan(self) -> None:
        with self._lock:
            self._last_plan_request_id = None
            self._last_start_state = None
            self._last_plan = None

    def execute_last_plan(self, node, execute_client, timeout_sec: float = 30.0):
        with self._lock:
            if self._last_plan is None:
                raise NoCachedPlanError("No cached plan available. Call /plan first.")

            planned_trajectory = self._last_plan
            plan_request_id = self._last_plan_request_id

        started = time.time()

        goal = ExecuteTrajectory.Goal()
        goal.trajectory = planned_trajectory

        send_future = execute_client.send_goal_async(goal)
        rclpy.spin_until_future_complete(node, send_future, timeout_sec=timeout_sec)
        goal_handle = send_future.result()

        if goal_handle is None or not goal_handle.accepted:
            raise ExecuteRejectedError("MoveIt rejected execute goal")

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(node, result_future, timeout_sec=timeout_sec)
        result_wrap = result_future.result()

        if result_wrap is None:
            raise ExecuteTimeoutError("Execution did not return before timeout")

        result = result_wrap.result
        error_val = int(result.error_code.val)
        execute_success = (error_val == 1)
        elapsed_ms = int((time.time() - started) * 1000)

        if execute_success:
            # Policy: clear cached plan only after successful execute.
            self.clear_plan()

        return {
            "execute_success": execute_success,
            "error_code": error_val,
            "message": "Execution success" if execute_success else "Execution failed",
            "duration_ms": elapsed_ms,
            "executed_plan_request_id": plan_request_id,
        }
```

### `rviz_visual.py`

```python
from moveit_msgs.msg import DisplayTrajectory, RobotState, RobotTrajectory
from rclpy.node import Node


class RvizVisualizer:
    def __init__(
        self,
        node: Node,
        topic: str = "/display_planned_path",
        qos_depth: int = 10,
    ) -> None:
        self._pub = node.create_publisher(DisplayTrajectory, topic, qos_depth)

    def publish_planned_trajectory(
        self,
        trajectory_start: RobotState,
        planned_trajectory: RobotTrajectory,
        model_id: str = "",
    ) -> None:
        msg = DisplayTrajectory()
        msg.model_id = model_id
        msg.trajectory_start = trajectory_start
        msg.trajectory = [planned_trajectory]
        self._pub.publish(msg)
```

### `server.py`

```python
import logging
import time
import uuid

import rclpy
import uvicorn
from fastapi import FastAPI, HTTPException, Request
from moveit_msgs.action import ExecuteTrajectory, MoveGroup
from rclpy.action import ActionClient
from rclpy.node import Node

from .constants import EXECUTION_TIMEOUT_SEC, JOINT_ORDER, PLANNING_TIMEOUT_SEC
from .execution import (
    ExecuteError,
    ExecuteRejectedError,
    ExecuteTimeoutError,
    ExecutionManager,
    NoCachedPlanError,
)
from .mapping import build_move_group_goal
from .models import PlanRequest
from .moveit_handler import MoveItHandler, MoveItHandlerError
from .rviz_visual import RvizVisualizer


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("moveit_solver_server")


class MoveItBridge:
    def __init__(self):
        if not rclpy.ok():
            rclpy.init(args=None)
        self.node = Node("moveit_solver_server_node")
        self.move_group_client = ActionClient(self.node, MoveGroup, "/move_action")
        self.execute_client = ActionClient(self.node, ExecuteTrajectory, "/execute_trajectory")
        self.ready = False
        self.execute_ready = False

    def connect(self, timeout_sec: float = 3.0) -> bool:
        self.ready = self.move_group_client.wait_for_server(timeout_sec=timeout_sec)
        return self.ready

    def connect_execute(self, timeout_sec: float = 3.0) -> bool:
        self.execute_ready = self.execute_client.wait_for_server(timeout_sec=timeout_sec)
        return self.execute_ready


app = FastAPI()
bridge = MoveItBridge()
moveit_handler = MoveItHandler()
rviz_visualizer = RvizVisualizer(bridge.node)
execution_manager = ExecutionManager()


def _planning_time_to_sec(value) -> float:
    if isinstance(value, (int, float)):
        return float(value)
    sec = getattr(value, "sec", 0)
    nanosec = getattr(value, "nanosec", 0)
    return float(sec) + float(nanosec) * 1e-9


@app.on_event("startup")
def startup_event():
    ready = bridge.connect(timeout_sec=3.0)
    execute_ready = bridge.connect_execute(timeout_sec=3.0)
    logger.info("moveit_bridge_ready=%s execute_bridge_ready=%s", ready, execute_ready)


@app.on_event("shutdown")
def shutdown_event():
    moveit_handler.close()


@app.get("/health")
def health():
    return {"ok": True}


@app.get("/ready")
def ready():
    bridge.connect(timeout_sec=0.5)
    bridge.connect_execute(timeout_sec=0.5)
    return {"moveit_ready": bridge.ready, "execute_ready": bridge.execute_ready}


@app.get("/current")
def current_state():
    try:
        result = moveit_handler.get_current_in_order(JOINT_ORDER, timeout_sec=2.0)
        return {
            "joint_names": result.joint_names,
            "current": result.current,
            "source": result.source,
            "units": result.units,
        }
    except MoveItHandlerError as exc:
        raise HTTPException(status_code=503, detail=str(exc))


@app.get("/current_pose")
def current_pose(link_name: str = "spray_cone.stp", frame_id: str = "world"):
    try:
        result = moveit_handler.get_current_pose(
            JOINT_ORDER,
            link_name=link_name,
            frame_id=frame_id,
            timeout_sec=2.0,
        )
        return {
            "frame_id": result.frame_id,
            "link_name": result.link_name,
            "position": result.position,
            "orientation": result.orientation,
            "source": result.source,
        }
    except MoveItHandlerError as exc:
        raise HTTPException(status_code=503, detail=str(exc))


@app.post("/plan")
def plan(request: PlanRequest, http_request: Request):
    request_id = http_request.headers.get("x-request-id") or str(uuid.uuid4())
    started = time.time()
    logger.info("request_received request_id=%s frame_id=%s", request_id, request.frame_id)

    if not bridge.connect(timeout_sec=0.5):
        raise HTTPException(status_code=503, detail="MoveIt backend not ready")

    try:
        goal_msg = build_move_group_goal(request)

        send_future = bridge.move_group_client.send_goal_async(goal_msg)
        rclpy.spin_until_future_complete(
            bridge.node, send_future, timeout_sec=PLANNING_TIMEOUT_SEC
        )
        goal_handle = send_future.result()

        if goal_handle is None or not goal_handle.accepted:
            elapsed_ms = int((time.time() - started) * 1000)
            return {
                "request_id": request_id,
                "plan_success": False,
                "error_code": "GOAL_REJECTED",
                "message": "MoveIt rejected planning goal",
                "duration_ms": elapsed_ms,
            }

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(
            bridge.node, result_future, timeout_sec=PLANNING_TIMEOUT_SEC
        )
        result_wrap = result_future.result()

        if result_wrap is None:
            elapsed_ms = int((time.time() - started) * 1000)
            return {
                "request_id": request_id,
                "plan_success": False,
                "error_code": "PLANNING_TIMEOUT",
                "message": "Planning did not return before timeout",
                "duration_ms": elapsed_ms,
            }

        result = result_wrap.result
        error_val = int(result.error_code.val)
        plan_success = (error_val == 1)

        traj_points = len(result.planned_trajectory.joint_trajectory.points)
        planning_time_sec = _planning_time_to_sec(result.planning_time)
        elapsed_ms = int((time.time() - started) * 1000)

        response = {
            "request_id": request_id,
            "plan_success": plan_success,
            "error_code": error_val,
            "message": "Planning success" if plan_success else "Planning failed",
            "duration_ms": elapsed_ms,
            "planning_time_sec": planning_time_sec,
            "trajectory_points": traj_points,
        }

        if plan_success:
            execution_manager.store_plan(
                request_id=request_id,
                trajectory_start=result.trajectory_start,
                planned_trajectory=result.planned_trajectory,
            )
            rviz_visualizer.publish_planned_trajectory(
                trajectory_start=result.trajectory_start,
                planned_trajectory=result.planned_trajectory,
                model_id="",
            )
            response["cached_for_execute"] = True

        logger.info(
            "request_finished request_id=%s plan_success=%s error_code=%d duration_ms=%d points=%d",
            request_id,
            plan_success,
            error_val,
            elapsed_ms,
            traj_points,
        )
        return response

    except HTTPException:
        raise
    except Exception as exc:
        elapsed_ms = int((time.time() - started) * 1000)
        logger.exception("request_failed request_id=%s duration_ms=%d", request_id, elapsed_ms)
        raise HTTPException(status_code=500, detail=f"Internal planning bridge error: {exc}")


@app.post("/execute")
def execute(http_request: Request):
    request_id = http_request.headers.get("x-request-id") or str(uuid.uuid4())
    logger.info("execute_request_received request_id=%s", request_id)

    if not bridge.connect_execute(timeout_sec=0.5):
        raise HTTPException(status_code=503, detail="MoveIt execute backend not ready")

    try:
        result = execution_manager.execute_last_plan(
            node=bridge.node,
            execute_client=bridge.execute_client,
            timeout_sec=EXECUTION_TIMEOUT_SEC,
        )
        result["request_id"] = request_id
        logger.info(
            "execute_request_finished request_id=%s execute_success=%s error_code=%s duration_ms=%s",
            request_id,
            result.get("execute_success"),
            result.get("error_code"),
            result.get("duration_ms"),
        )
        return result
    except NoCachedPlanError as exc:
        raise HTTPException(status_code=409, detail=str(exc))
    except (ExecuteRejectedError, ExecuteTimeoutError) as exc:
        raise HTTPException(status_code=503, detail=str(exc))
    except ExecuteError as exc:
        raise HTTPException(status_code=500, detail=str(exc))


def main():
    uvicorn.run(app, host="172.21.1.108", port=8000)
```

