# Linear Actuator Test 程序说明 / Program Guide

## 1. 程序目标 / Program Objectives

**中文**

`linear_act_test.ino` 用于在 Arduino Mega 上同时控制 4 个带位置传感器的线性执行器。程序需要：

1. 上电后自动判断每个执行器的伸出和缩回方向。
2. 四路方向校准同时进行，不逐个等待。
3. 校准完成后，四路同时移动到 50 mm 的初始待机位置。
4. 收到上位机目标后，四路同时向各自目标移动。
5. 每路独立判断是否到位。某一路完成后只停止该路，其他执行器继续运动。

Arduino Mega 没有原生多线程操作系统，因此程序使用非阻塞的协作式状态机。四路在每次主循环中分别更新自己的状态，从而实现并行效果。

**English**

`linear_act_test.ino` controls four position-feedback linear actuators concurrently on an Arduino Mega. The program must:

1. Detect the extend and retract direction of every actuator automatically at startup.
2. Calibrate all four actuator directions concurrently rather than one at a time.
3. Move all four actuators concurrently to the initial 50 mm standby position after calibration.
4. Move all four actuators concurrently toward their individual targets after receiving a host command.
5. Evaluate completion independently. When one actuator reaches its target, only that actuator stops while the others continue moving.

The Arduino Mega does not provide a native multithreaded operating system. The program therefore uses non-blocking cooperative state machines. Each actuator updates its own state during every main-loop cycle, producing concurrent behavior without operating-system threads.

## 2. 硬件对应关系 / Hardware Mapping

**中文**

传感器、执行器和 PWM 输出按数组顺序严格一一对应。修改引脚时，必须保持三个数组的下标关系。

**English**

Each sensor, actuator, and PWM pair must share the same array index. Any future pin changes must preserve the index relationship among all three arrays.

| 执行器 / Actuator | 程序下标 / Index | 传感器 / Sensor | RPWM | LPWM |
| --- | ---: | ---: | ---: | ---: |
| 执行器 / Actuator 1 | 0 | A0 | 2 | 3 |
| 执行器 / Actuator 2 | 1 | A1 | 4 | 5 |
| 执行器 / Actuator 3 | 2 | A2 | 6 | 7 |
| 执行器 / Actuator 4 | 3 | A3 | 8 | 9 |

```cpp
const int SENSOR_PINS[4] = {A0, A1, A2, A3};
const int RPWM_PINS[4] = {2, 4, 6, 8};
const int LPWM_PINS[4] = {3, 5, 7, 9};
```

## 3. 启动与方向校准 / Startup and Direction Calibration

**中文**

上电后的处理顺序：

1. 初始化 8 个 PWM 引脚并停止全部电机。
2. 四路同时使用 RPWM 以 PWM 200 驱动 2 秒，记录各自的传感器趋势。
3. 全部停止 200 ms。
4. 四路同时使用 LPWM 以 PWM 200 驱动 2 秒，再次记录趋势。
5. 根据传感器增减方向，独立得到每路的 `rpwmIsExtend`。

校准使用独立状态机，不使用阻塞式 `delay()` 或长时间 `while` 循环。如果某路在两个方向上的变化都小于 2 个 ADC 单位，该路会被标记为传感器无响应，并且不参与后续位置控制。

**English**

The startup sequence is:

1. Initialize all eight PWM pins and stop every motor.
2. Drive all four RPWM outputs concurrently at PWM 200 for two seconds and record each sensor trend.
3. Stop all outputs for 200 ms.
4. Drive all four LPWM outputs concurrently at PWM 200 for two seconds and record the trends again.
5. Determine `rpwmIsExtend` independently for each actuator from the direction of its sensor change.

Calibration uses independent state machines and contains no blocking `delay()` or long-running `while` loop. If an actuator changes by fewer than two ADC counts in both directions, its sensor is marked unresponsive and that actuator is excluded from subsequent position control.

```text
DBG sensor X not responding during calibration -> disabled
```

## 4. 50 mm 初始待机运动 / Initial Move to the 50 mm Standby Position

**中文**

方向校准期间不发送 JSON 位置状态。全部校准完成后，程序会：

1. 读取四路当前位置。
2. 发送一次 JSON 现状。
3. 将四路目标设为 50 mm。
4. 同时启动所有校准有效的执行器。
5. 每路进入 50 ± 2 mm 范围后，只将该路的 RPWM 和 LPWM 置零。

某一路完成不会修改其他执行器的独立运行标志。只有所有有效执行器都停止后，整体 `targetsActive` 才变为 `false`。

**English**

No JSON position telemetry is sent during direction calibration. After all calibration tasks finish, the program:

1. Reads the current position of all four channels.
2. Sends one JSON status report.
3. Sets all four targets to 50 mm.
4. Starts every successfully calibrated actuator concurrently.
5. Sets only that actuator's RPWM and LPWM outputs to zero when it enters the 50 ± 2 mm range.

Completion of one actuator does not change any other actuator's independent motion flag. The global `targetsActive` flag becomes `false` only after every valid actuator has stopped.

## 5. 目标命令与后续运动 / Target Commands and Normal Motion

**中文**

上位机使用一行 CSV 命令设置四路目标，每个目标必须位于 0–800 mm。收到合法命令后，四路同时向各自目标移动。

**English**

The host sets four targets with one CSV command. Every target must be within 0–800 mm. After a valid command is received, all four actuators move concurrently toward their individual targets.

```text
T,<a1_target>,<a2_target>,<a3_target>,<a4_target>
```

示例 / Example:

```text
T,100,150,200,250
```

校准完成前收到命令时 / If a command arrives before calibration is complete:

```json
{"error":"calibration_in_progress"}
```

**中文**

远离目标时使用固定 PWM 200；距离目标不超过 10 mm 时切换为 PID 调速。当误差进入 ±2 mm 死区后，该路立即停止并退出本次运动。

**English**

The controller uses a fixed PWM value of 200 when far from the target. Within 10 mm, it switches to PID speed control to reduce overshoot. Once the error enters the ±2 mm deadband, that actuator stops immediately and exits the current motion task.

## 6. 传感器处理与位置换算 / Sensor Processing and Position Conversion

**中文**

每路传感器使用最近 5 个样本的中值滤波。正常运动阶段每 100 ms 更新一次传感器和电机控制。当前固定标定为：

- ADC 原始值 3 对应 0 mm。
- ADC 原始值 812 对应 800 mm。
- 中间数值按线性比例换算。

标定值必须与实际传感器量程和安装方式一致，否则程序可能误判已经到位。

**English**

Each channel uses a median filter over its five most recent sensor samples. During normal motion, sensor readings and motor commands update every 100 ms. The current fixed calibration is:

- Raw ADC value 3 corresponds to 0 mm.
- Raw ADC value 812 corresponds to 800 mm.
- Intermediate values are converted linearly.

These calibration values must match the actual sensor range and installation. Otherwise, the program may incorrectly conclude that an actuator has reached its target.

## 7. 堵转监测与停机原则 / Stall Monitoring and Stop Rules

**中文**

每路使用原始 ADC 变化独立监测运动进度。如果 3 秒内累积变化不足 2 个 ADC 单位，程序输出一次堵转警告。当前 `STOP_ON_STALL = false`，因此警告不会停止电机，执行器默认继续向自己的目标运动。

**English**

Each actuator independently monitors motion progress using raw ADC changes. If the accumulated change remains below two ADC counts for three seconds, the program prints one stall warning. Because `STOP_ON_STALL = false`, this warning does not stop the motor; by default, the actuator continues toward its own target.

```text
DBG actuator X stall warning current=... target=...
```

```cpp
const bool STOP_ON_STALL = false;
```

**中文**

如将该开关改为 `true`，堵转超时将只停止发生问题的单路，不影响其他执行器。

**English**

If this option is changed to `true`, a stall timeout stops only the affected actuator and does not stop the other channels.

## 8. JSON 状态上报 / JSON Telemetry

**中文**

方向校准完成后，程序每 1 秒上报一次四路当前位置和目标位置。`a1`–`a4` 是上位机使用的执行器编号，分别对应程序下标 0–3。

**English**

After direction calibration, the program reports all four current and target positions once per second. Host-side names `a1` through `a4` correspond to program array indices 0 through 3.

```json
{
  "a1_current": 50,
  "a1_target": 50,
  "a2_current": 50,
  "a2_target": 50,
  "a3_current": 50,
  "a3_target": 50,
  "a4_current": 50,
  "a4_target": 50
}
```

## 9. 只有一路运动时的检查 / Troubleshooting When Only One Actuator Moves

**中文**

1. 查看是否有 `sensor X ... disabled`；如果有，该路已被校准逻辑排除。
2. 查看是否有 `actuator X reached target`；如果有，程序认为该路已进入目标死区。
3. 比较 JSON 中的 `current` 和 `target`，确认传感器分别反映对应执行器的实际位置。
4. 确认 A0→2/3、A1→4/5、A2→6/7、A3→8/9 的一一对应接线。
5. 如果软件仍在输出 PWM，但电机不动，检查 H 桥使能端、共地、PWM 接线和多台电机同时启动时的电源压降。

**English**

1. Check for `sensor X ... disabled`; if present, calibration excluded that channel.
2. Check for `actuator X reached target`; if present, the controller believes that channel is already inside the target deadband.
3. Compare every JSON `current` and `target` value and verify that each sensor reflects the physical position of its corresponding actuator.
4. Confirm the one-to-one wiring: A0→2/3, A1→4/5, A2→6/7, and A3→8/9.
5. If software continues producing PWM but a motor does not move, inspect the H-bridge enable inputs, common ground, PWM wiring, and supply-voltage drop when several motors start together.

## 10. 当前验证状态 / Current Verification Status

**中文**

程序已使用 Arduino CLI 针对 `arduino:avr:mega` 通过编译检查。编译通过代表语法、库依赖和 Arduino Mega 目标配置正常。电机方向、传感器标定、PID 参数、电源能力和机械限位仍需要在实际硬件上验证。

**English**

The sketch has passed an Arduino CLI compile check for `arduino:avr:mega`. This verifies its syntax, library dependencies, and Arduino Mega target configuration. Motor direction, sensor calibration, PID tuning, power capacity, and mechanical limits must still be verified on the physical system.

## 11. 尚未完成的改进与待讨论事项 / Open Improvements and Topics for Discussion

> 本节记录尚未实现或尚未确定的改进方向，不代表当前程序已经具备这些功能。
>
> This section records improvements that have not yet been implemented or finalized. It does not describe functions already guaranteed by the current program.

### 11.1 考虑改为逐路方向校准 / Consider Sequential Direction Calibration

**中文**

当前程序让四路执行器同时进行方向校准。待评估的改进方案是：校准时一次只驱动一个执行器，其他三路保持停止，按执行器 1–4 依次完成校准。

这种方式可以直接验证电机与传感器的一一对应关系：驱动某一路 PWM 时，只有对应的传感器应该出现明显变化。如果另一个传感器变化，可以判定为接线或通道映射错误；如果没有传感器变化，可能是传感器断联、驱动器异常或电机未运动。

优点是诊断更可靠、同时启动电流更小，也能降低错误接线导致异常动作的风险。缺点是启动校准时间会由约 4.4 秒增加到约 17.6 秒（按当前每路两个 2 秒驱动阶段和两个 200 ms 等待阶段估算）。是否改为逐路校准尚待确认。

**English**

The current program calibrates all four actuator directions concurrently. A proposed improvement is to energize only one actuator at a time while the other three remain stopped, completing calibration sequentially from Actuator 1 through Actuator 4.

This approach can directly verify the one-to-one relationship between each motor and sensor. When one PWM pair is energized, only its assigned sensor should show significant movement. Movement on a different sensor indicates a wiring or channel-mapping error. No sensor movement may indicate a disconnected sensor, faulty driver, or motor that did not move.

The advantages are more reliable diagnostics, lower simultaneous startup current, and a lower risk of abnormal movement caused by incorrect wiring. The disadvantage is a longer startup calibration time: approximately 17.6 seconds instead of 4.4 seconds with the current timings. The decision to adopt sequential calibration remains open.

### 11.2 确认 JSON 通信规格 / Confirm the JSON Communication Contract

**中文**

当前 JSON 格式仍需要与 Python/Grasshopper 上位机程序联合确认，包括：

- 字段名是否继续使用 `a1_current`–`a4_target`。
- 位置数值的单位是 mm 还是原始 ADC 值。
- 是否增加消息类型、时间戳、控制器/节点编号、执行器编号和运行状态。
- 错误、警告和正常状态是否使用统一的 JSON 包结构。
- 一行是否始终只包含一个完整 JSON 对象，以方便上位机按行解析。
- `DBG` 文本是否应在正式运行中关闭，或改为结构化 JSON，避免与机器可读数据混在同一串口中。

在上述项目确定前，当前 JSON 格式只应视为临时通信协议。

**English**

The JSON format still needs to be confirmed jointly with the Python/Grasshopper host software. Open questions include:

- Whether field names should remain `a1_current` through `a4_target`.
- Whether position values represent millimetres or raw ADC readings.
- Whether to add a message type, timestamp, controller/node ID, actuator ID, and operating state.
- Whether status, warning, and error messages should share one consistent JSON envelope.
- Whether each serial line must always contain exactly one complete JSON object for reliable line-based parsing.
- Whether plain-text `DBG` messages should be disabled in production or converted to structured JSON so that machine-readable data is not mixed with debug text on the same serial connection.

Until these decisions are finalized, the current JSON format should be treated as a provisional communication contract.

### 11.3 盘点并扩展可上报的 JSON 消息 / Inventory and Extend Reportable JSON Messages

**中文**

当前程序实际输出的 JSON 只有以下两类：

1. **位置状态**：四路 `current` 和 `target`。
2. **错误对象**：通过 `{"error":"<code>"}` 输出，现有错误码为：
   - `empty_message`：收到空命令。
   - `calibration_in_progress`：校准未完成时收到目标命令。
   - `invalid_command`：命令格式或目标范围无效。
   - `input_overflow`：串口输入超过缓冲区长度。
   - `sensor_not_connected`：方向校准后至少有一路传感器无明显响应。

以下信息当前只是 `DBG` 文本，尚未作为 JSON 上报：

- 单路方向校准结果。
- 传感器无响应时的具体执行器编号。
- 执行器到达目标。
- 堵转警告，包括当前位置和目标位置。
- 整组执行器全部停止或本次运动完成。
- 开机、校准中、待机、运动中、警告、紧急停止等总体状态。

建议后续将所有机器可读消息统一为带 `type`、`code`、`severity`、`actuator`、`current`、`target` 和 `message` 等字段的 JSON。例如，下列仅是待讨论的提案，尚未实现：

```json
{"type":"warning","code":"stall_detected","severity":"warning","actuator":3,"current":120,"target":300}
```

```json
{"type":"error","code":"sensor_not_connected","severity":"error","actuator":2}
```

```json
{"type":"event","code":"target_reached","severity":"info","actuator":1,"current":50,"target":50}
```

**English**

The current program actually emits only two categories of JSON:

1. **Position telemetry:** `current` and `target` for all four channels.
2. **Error objects:** emitted as `{"error":"<code>"}`. Existing error codes are:
   - `empty_message`: an empty command was received.
   - `calibration_in_progress`: a target command arrived before calibration completed.
   - `invalid_command`: the command syntax or target range is invalid.
   - `input_overflow`: serial input exceeded the buffer length.
   - `sensor_not_connected`: at least one sensor showed no significant response during direction calibration.

The following information currently exists only as plain-text `DBG` output and is not yet reported as JSON:

- Per-actuator direction-calibration results.
- The exact actuator index associated with an unresponsive sensor.
- Target-reached events.
- Stall warnings, including current and target positions.
- Completion or stopped state for the whole actuator group.
- Overall states such as startup, calibrating, standby, moving, warning, and emergency stop.

A future revision should consider a consistent JSON structure containing fields such as `type`, `code`, `severity`, `actuator`, `current`, `target`, and `message`. The examples above are proposals only and have not been implemented.

### 11.4 异常高度差与自动紧急停止 / Abnormal Height Difference and Automatic Emergency Stop

**中文**

系统需要增加异常时的自动紧急停止，避免相邻执行器高度差过大而拉扯或扯断连接件。尚需要确定：

- 允许的最大相邻高度差。
- 警告阈值和紧急停止阈值是否分开。
- 对比实际位置、目标位置，还是同时对比两者。
- 传感器失效、数据超范围、通信中断、堵转和高度差超限各自应触发何种停机级别。
- 紧急停止后如何复位，是否必须由人工明确解除，而不能自动恢复运动。

紧急停止的决策位置有三种可选方案：

1. **Arduino 本地决策**：优点是响应快，不依赖 USB、Python 或 Grasshopper；缺点是每块 Arduino 只看得到本节点的 4 路位置，无法单独判断不同 Arduino 节点之间的相邻高度差。
2. **Grasshopper/上位机决策**：优点是可以监看全部节点和整个曲面的邻接关系；缺点是依赖串口上报频率、USB 连接、Python 服务和 Grasshopper 循环，延迟和通信故障可能导致停机不及时。
3. **分层联合决策**：Arduino 负责本节点的快速保护，Grasshopper 负责全局高度差和跨节点邻接关系。任意一层发现严重异常都可发出停机命令。

当前更值得继续讨论的是第 3 种分层方案。Arduino 不应只等待 Grasshopper 的停机命令；它应至少能够在本地检测传感器严重异常、本节点高度差超限或通信心跳超时，立即将全部 PWM 置零并锁定在紧急停止状态。Grasshopper 则根据全局曲面数据发现跨节点异常，向所有相关 Arduino 广播紧急停止命令。

这个方案仍需要讨论和定义命令格式、心跳时间、高度差阈值、紧急停止的 JSON 上报、锁定与手动复位流程。在这些安全功能完成之前，系统不应依赖当前软件作为唯一的机械安全保护。

**English**

The system needs an automatic emergency-stop function to prevent excessive height differences between neighboring actuators from stretching or breaking their connecting components. The following items remain to be defined:

- The maximum permitted height difference between neighbors.
- Whether warning and emergency-stop thresholds should be separate.
- Whether protection should compare actual positions, commanded targets, or both.
- Which stop level should be triggered by sensor failure, out-of-range data, communication loss, stall, and excessive height difference.
- How the system is reset after an emergency stop, including whether an explicit manual reset must always be required instead of automatic motion recovery.

There are three possible locations for the emergency-stop decision:

1. **Local Arduino decision:** fast and independent of USB, Python, and Grasshopper, but each Arduino can see only its own four channels and cannot independently evaluate neighbor differences across controller nodes.
2. **Grasshopper/host decision:** has visibility of all nodes and the complete surface-neighbor topology, but depends on telemetry rate, USB links, Python services, and the Grasshopper update loop. Latency or communication failure could delay a stop.
3. **Layered joint decision:** Arduino provides fast local protection, while Grasshopper monitors global height differences and cross-node neighbor relationships. Either layer may request a stop when it detects a serious fault.

The third, layered approach is currently the strongest candidate for further discussion. Arduino should not merely wait for a Grasshopper stop command. At minimum, it should be able to detect severe local sensor faults, excessive local height differences, or a communication-heartbeat timeout, immediately set all PWM outputs to zero, and latch an emergency-stop state. Grasshopper should detect global and cross-node surface faults and broadcast an emergency-stop command to all affected Arduino controllers.

This design still requires agreement on the command format, heartbeat interval, height-difference thresholds, emergency-stop JSON report, latching behavior, and manual reset procedure. Until these safety functions are implemented, the current software should not be treated as the sole mechanical safety system.
