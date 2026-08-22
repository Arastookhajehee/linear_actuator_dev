# Linear Actuator Test AI Discussion Record

## 中文 / Chinese

### 1. 背景与目标
本次讨论围绕 Arduino Mega 控制的线性执行器测试程序展开，目标是排查启动后某个执行器持续运行、方向校准结果异常，以及传感器反馈不稳定的问题。

### 2. 发现的问题
在程序运行后，串口输出显示了方向校准阶段的日志，例如：

- `DBG starting direction calibration`
- `DBG cal actuator 0 rpwm_trend=1 lpwm_trend=0 rpwm_is_extend=1`
- `DBG cal actuator 1 rpwm_trend=0 lpwm_trend=1 rpwm_is_extend=0`
- `DBG cal actuator 2 rpwm_trend=-11 lpwm_trend=8 rpwm_is_extend=0`
- `DBG cal actuator 3 rpwm_trend=0 lpwm_trend=1 rpwm_is_extend=0`
- `DBG direction calibration complete`
- `DBG moving all actuators to 50mm`

其中一个执行器在校准完成后仍然表现为持续驱动，说明控制逻辑中存在“方向判断不稳”或“驱动没有正确进入停止状态”的问题。

### 3. 已处理的改进
#### 3.1 独立 Sketch 编译问题
原始文件和测试文件放在同一目录下时，Arduino 会把它们当作同一个 Sketch 一起编译，导致重复定义错误，例如：

- `ACTUATOR_COUNT`
- `SENSOR_PINS`
- `setup()` / `loop()`

解决方法是将测试文件移入独立子目录，作为单独的 Arduino Sketch 使用。

#### 3.2 增加“到位即停”保护
在运动控制逻辑中加入了以下保护：

- 当执行器到达目标位置时，立即停止驱动。
- 如果执行器长时间没有出现有效位移，则认为其发生卡滞或无响应。
- 输出调试信息并停止该执行器，避免持续驱动。

#### 3.3 增加“传感器无响应”判断
在方向校准阶段，如果某个传感器在两侧驱动下都没有明显变化，则程序会判定其为“未连接”或“无响应”状态，并在后续运动阶段跳过该执行器。

对应日志示例：

- `DBG sensor X not responding during calibration -> marked disconnected`

### 4. 结论
当前的控制逻辑已经从“只依赖方向判断”升级为更稳妥的流程：

1. 先进行方向校准。
2. 再判断传感器是否具有有效响应。
3. 发现无响应时，标记为未连接并跳过。
4. 发现卡滞时，自动停止，避免持续运行。

这使测试程序更适合定位问题来源：是驱动方向判断错误，还是传感器本身无反馈，还是机械结构卡住。

---

## English / 英文

### 1. Background and Objective
This discussion focused on the Arduino Mega-based linear actuator test program. The goal was to investigate why one actuator kept running continuously after startup, why the direction calibration behaved unexpectedly, and why some sensor feedback appeared unstable.

### 2. Problems Identified
After the program started, the serial output showed calibration logs such as:

- `DBG starting direction calibration`
- `DBG cal actuator 0 rpwm_trend=1 lpwm_trend=0 rpwm_is_extend=1`
- `DBG cal actuator 1 rpwm_trend=0 lpwm_trend=1 rpwm_is_extend=0`
- `DBG cal actuator 2 rpwm_trend=-11 lpwm_trend=8 rpwm_is_extend=0`
- `DBG cal actuator 3 rpwm_trend=0 lpwm_trend=1 rpwm_is_extend=0`
- `DBG direction calibration complete`
- `DBG moving all actuators to 50mm`

One actuator still behaved as if it were continuously driven after calibration, which suggested either an unstable direction decision or a failure to enter the stop state correctly.

### 3. Improvements Implemented
#### 3.1 Independent Sketch Compilation Issue
When the original file and the test file were placed in the same folder, Arduino treated them as one combined Sketch and caused redefinition errors such as:

- `ACTUATOR_COUNT`
- `SENSOR_PINS`
- `setup()` / `loop()`

The solution was to move the test file into its own folder so it could compile as a standalone Arduino Sketch.

#### 3.2 Added “Stop Immediately on Reach” Protection
The motion logic was improved so that:

- When an actuator reaches its target, it stops immediately.
- If an actuator shows no meaningful movement for a long period, it is considered stalled or unresponsive.
- A debug message is printed and the actuator is stopped to prevent endless driving.

#### 3.3 Added “Sensor Not Responding” Detection
During direction calibration, if a sensor shows no clear change after driving in both directions, the program marks it as disconnected or unresponsive and skips it in later motion phases.

Example log:

- `DBG sensor X not responding during calibration -> marked disconnected`

### 4. Conclusion
The control logic has been strengthened from a simple direction-based approach into a more robust workflow:

1. Perform direction calibration.
2. Check whether the sensor responds meaningfully.
3. If it does not respond, mark it as disconnected and skip it.
4. If it stalls, stop it automatically to avoid continuous motion.

This makes the test program more useful for identifying whether the issue comes from incorrect direction mapping, missing sensor feedback, or a mechanical jam.
