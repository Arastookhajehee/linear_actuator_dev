# ROS 2 Humble Installation - Step by Step

From [[Robotics]]

[Documentation](https://docs.ros.org/en/rolling/Installation/Ubuntu-Install-Debs.html)
## Environment

- Windows WSL Ubuntu 22.04

## Step 1: Set Locale

**Purpose**: Ensure UTF-8 locale support for ROS 2

```bash
locale && sudo apt update && sudo apt install locales && sudo locale-gen en_US en_US.UTF-8 && sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 && export LANG=en_US.UTF-8 && locale
```

**Expected**: After running, `locale` should show `LANG=en_US.UTF-8`

---

## Step 2: Setup Sources

**Purpose**: Add ROS 2 apt repository to your system

```bash
sudo apt install software-properties-common && sudo add-apt-repository universe && sudo apt update && sudo apt install curl -y && export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F'"' '{print $4}') && curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb" && sudo dpkg -i /tmp/ros2-apt-source.deb
```

---

## Step 3: Install ROS 2 Packages

**Purpose**: Install ROS 2 Humble on your system

```bash
sudo apt update && sudo apt upgrade && sudo apt install ros-humble-desktop
```

*(Alternative: `ros-humble-ros-base` for bare bones, `ros-dev-tools` for dev tools)*

---

## Step 4: Environment Setup

**Purpose**: Source ROS 2 environment

```bash
source /opt/ros/humble/setup.bash
```

---

## Step 5: Test Installation

**Purpose**: Verify ROS 2 is working

**Terminal 1 (Talker)**:
```bash
source /opt/ros/humble/setup.bash && ros2 run demo_nodes_cpp talker
```

**Terminal 2 (Listener)**:
```bash
source /opt/ros/humble/setup.bash && ros2 run demo_nodes_py listener
```

**Working behavior**: 
- Talker shows: `Publishing: "Hello World: ..."`
- Listener shows: `I heard: "Hello World: ..."`

---

*Installation complete*
