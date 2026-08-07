# MoveIt 2 Installation Guide (from source)

**Target**: Ubuntu 22.04 (WSL) with ROS 2 Humble

**Source**: https://moveit.picknik.ai/humble/doc/tutorials/getting_started/getting_started.html

---

## Steps Overview

1. Install ROS 2 and Colcon
2. Create colcon workspace
3. Download tutorials source
4. Download MoveIt source
5. Build workspace
6. Setup workspace
7. Test

---

## Step 1: Install Dependencies

Install rosdep:
```bash
sudo apt install -y python3-rosdep
sudo rosdep init
rosdep update
sudo apt update
sudo apt dist-upgrade
rosdep update
sudo apt install python3-rosdep
sudo rosdep init
rosdep update
sudo apt update
sudo apt dist-upgrade
```

Install Colcon:
```bash
sudo apt install -y python3-colcon-common-extensions python3-colcon-mixin
colcon mixin add default https://raw.githubusercontent.com/colcon/colcon-mixin-repository/master/index.yaml
colcon mixin update default
```

Install vcstool:
```bash
sudo apt install -y python3-vcstool
sudo apt update; sudo apt upgrade -y
```

---

## Step 2: Create Colcon Workspace

```bash
mkdir -p ~/ws_moveit2/src
cd ~/ws_moveit2/src
```

---

## Step 3: Download Tutorials Source

```bash
cd ~/ws_moveit2/src
git clone --branch humble https://github.com/ros-planning/moveit2_tutorials
```

---

## Step 4: Download MoveIt Source

```bash
cd ~/ws_moveit2/src
vcs import < moveit2_tutorials/moveit2_tutorials.repos
```

---

## Step 5: Build Workspace

Install dependencies and build:
```bash
cd ~/ws_moveit2
source /opt/ros/humble/setup.bash
rosdep install -r --from-paths . --ignore-src --rosdistro $ROS_DISTRO -y
colcon build --mixin release --parallel-workers 1
```

**Note**: Building can take 40-60 minutes depending on your CPU.
**Note**: the `--parallel-workers` limits the number of parallel processes and prevents OOM.

---

## Step 6: Setup Workspace

Source the workspace:
```bash
source ~/ws_moveit2/install/setup.bash
```

Optional: Add to `.bashrc` for auto-sourcing:
```bash
echo 'source ~/ws_moveit2/install/setup.bash' >> ~/.bashrc
```

---

## Step 7: Test

Launch the demo:
```bash
source /opt/ros/humble/setup.bash
source ~/ws_moveit2/install/setup.bash
ros2 launch moveit2_tutorials demo.launch.py
```
