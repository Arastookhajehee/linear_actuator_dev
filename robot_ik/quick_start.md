# Moveit and Server Quick Start Scripts

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
