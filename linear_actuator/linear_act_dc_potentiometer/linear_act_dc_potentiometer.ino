/*
  A1L / A1S - Linear Actuator Control (IBT-2) for Arduino (REWRITE)
  ---------------------------------------------------------------
  Pins:
    Sensor (A1S): A0
    IBT-2 PWM:    D5 (RPWM), D6 (LPWM)
  Serial:
    9600 baud
    input target: integer 0..1000 (CR/LF terminated)

  Filter (ALL phases):
    - Short-window MODE / majority vote with tolerance (±1 clustering)
      Goal: suppress occasional ±1 jitter and output "most credible" integer.

  Init:
    Step1 Direction confirm (longer travel, 2 reciprocations):
      RPWM LEG -> LPWM LEG -> RPWM LEG -> LPWM LEG
      Observe sensor delta; whichever increases sensor is EXTEND.
    Step2 Homing:
      Retract continuously.
      Use "trend + stall" (window range small for confirm duration).
      Then keep retracting 0.2s, stop.
      Set zeroScaled from stable samples at the end (robust).

  Control:
    - Deadband = 2
    - PID_ENABLE_WINDOW enlarged (near-zone PID)
    - PID gains updated (Kp higher, small Ki, small Kd)
    - MIN_PWM is applied as breakaway / not-too-small-error assist (NOT always-on)

  Completion:
    When |error| <= deadband AND position stable for 3s:
      stop motor, print "A1 DONE"
      enter HOLD_DONE:
        - report every 2s
        - do NOT move based on sensor
        - accept new target; if differs enough, start MOVING
*/

#include <Arduino.h>

// -------------------- Pins --------------------
// EN Analog sensor input pin (A1S). Reads 0..1023 by analogRead(). 
// CN 模拟传感器输入引脚（A1S），analogRead() 读取范围 0..1023。
const int A1S_PIN  = A0;

// EN IBT-2 motor driver PWM pins: RPWM and LPWM (two directions). 
// CN IBT-2 电机驱动的 PWM 引脚：RPWM/LPWM（两路分别对应两个方向）。
const int RPWM_PIN = 5;
const int LPWM_PIN = 6;

// -------------------- Serial --------------------
// EN Serial baud rate for command/telemetry. 
// CN 串口波特率，用于接收目标与输出状态。
const long SERIAL_BAUD = 9600;

// EN Serial line buffer length (target input). 
// CN 串口输入缓冲区长度（接收一行目标值）。
const int SERIAL_BUFFER_LEN = 16;
char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;

// EN If user inputs a line longer than buffer, mark overflow and reject. 
// CN 如果输入一行超过缓冲区长度，标记溢出并拒绝该行。
bool serialLineOverflow = false;

// -------------------- Timing --------------------
// EN Control loop period while MOVING (ms). 
// CN 运动状态下控制循环周期（毫秒）。
const unsigned long CONTROL_DT_MS = 20;

// EN Report period while moving: frequent telemetry. 
// CN 运动时上报周期：更频繁输出位置。
const unsigned long REPORT_DT_MS_MOVING = 100;

// EN Report period while holding done: slow telemetry. 
// CN 完成并保持时上报周期：降低输出频率。
const unsigned long REPORT_DT_MS_HOLD   = 2000;

unsigned long lastControlMs = 0;
unsigned long lastReportMs  = 0;

// -------------------- PWM --------------------
// EN Max PWM value for Arduino analogWrite() (0..255). 
// CN Arduino analogWrite() 的 PWM 最大值（0..255）。
const int PWM_MAX  = 255;

// EN Init/homing/test speed PWM (tune for your actuator/driver). 
// CN 初始化/回零/方向确认时使用的 PWM（需要结合执行器/驱动调参）。
const int PWM_INIT = 160;

// EN Minimum PWM used as "breakaway" assistance to overcome stiction. 
// CN 最小 PWM（“起动克服静摩擦”辅助），避免卡在很小输出下不动。
const int MIN_PWM  = 45;

// -------------------- Scaling --------------------
// EN Convert raw analogRead(0..1023) to scaled 0..1000 integer. 
// CN 将 analogRead 原始值(0..1023) 映射为 0..1000 的整数刻度。
static inline int rawToScaled1000(int raw) {
  long v = (long)raw * 1000L;
  v /= 1023L;
  if (v < 0) v = 0;
  if (v > 1000) v = 1000;
  return (int)v;
}

// ============================================================================
// Filter: Short-window MODE / majority vote (with ±1 tolerance clustering)
// ============================================================================
// EN Rationale:
//    Take N quick samples, then choose the "most supported" value cluster (±1).
//    This strongly suppresses occasional ±1 jitter, while still tracking steps.
// CN 原理说明：
//    快速采 N 个样本，选择“被最多样本支持”的值簇（±1 认为同簇）。
//    能强力抑制偶发 ±1 抖动，同时对真实跳变也能较快跟随。

// EN Last filtered value is used as tie-breaker preference (hysteresis). 
// CN 上一次滤波输出用于平分时的偏好（类似轻微滞回，减少来回跳）。
static int lastFilteredScaled = 0;

int readScaled1000ModeVote()
{
  // EN Short window size. Larger = more robust but slower response. 
  // CN 短窗长度。越大越稳但响应越慢。
  const int N = 11;
  int s[N];

  // EN Collect samples quickly; small delay lets ADC settle a bit. 
  // CN 快速采样；微小延时可让 ADC 稳定一点点。
  for (int i = 0; i < N; i++) {
    s[i] = rawToScaled1000(analogRead(A1S_PIN));
    delayMicroseconds(200);
  }

  int bestIdx = 0;
  int bestScore = -1;

  // EN Score each candidate center by counting samples within ±1. 
  // CN 对每个候选中心计分：统计落在 ±1 范围内的样本个数。
  for (int i = 0; i < N; i++) {
    int score = 0;
    int center = s[i];

    for (int j = 0; j < N; j++) {
      if (abs(s[j] - center) <= 1) score++;   // EN ±1 = same cluster // CN ±1 视为同一可信簇
    }

    // EN Pick highest score; if tie, pick closer to last output. 
    // CN 选择最高分；若平分，选更接近上一次输出的中心。
    if (score > bestScore) {
      bestScore = score;
      bestIdx = i;
    } else if (score == bestScore) {
      if (abs(center - lastFilteredScaled) < abs(s[bestIdx] - lastFilteredScaled)) {
        bestIdx = i;
      }
    }
  }

  // EN Output the chosen cluster center. 
  // CN 输出所选的簇中心值。
  int out = s[bestIdx];
  lastFilteredScaled = out;
  return out;
}

// ============================================================================
// Coordinate system
// ============================================================================
// EN zeroScaled is the sensor reading at the mechanical minimum end (home). 
// CN zeroScaled 表示机械最小端（回零点）时的传感器读数（scaled）。
int zeroScaled = 0;

// EN pos1000 is runtime position in 0..1000 (relative to zeroScaled). 
// CN pos1000 为运行时位置（相对 zeroScaled 后的 0..1000）。
int pos1000 = 0;

// EN target1000 is the commanded target position 0..1000 from Serial. 
// CN target1000 为串口输入的目标位置 0..1000。
int target1000 = 0;

// EN Read position: filtered scaled minus zero offset, then clamp 0..1000. 
// CN 读取位置：滤波后的 scaled 减去零点偏置，再夹紧到 0..1000。
int readPos1000()
{
  int scaled = readScaled1000ModeVote();
  int p = scaled - zeroScaled;
  if (p < 0) p = 0;
  if (p > 1000) p = 1000;
  return p;
}

// -------------------- Deadband & completion stability --------------------
// EN Deadband threshold: within this error, we treat as "arrived". 
// CN 死区阈值：误差小于等于该值时认为到位（不再强行逼近）。
const int POS_DEADBAND_INT = 1;

// EN Stability condition: position must stay within ±1 for 3 seconds. 
// CN 稳定判定：位置在 ±1 内保持 3 秒才算真正完成。
const int   STABLE_POS_TOL = 1;
const unsigned long STABLE_TIME_MS = 3000;

// EN In HOLD_DONE, new target must differ enough to restart movement. 
// CN HOLD_DONE 状态下，新目标偏差达到一定值才重新进入运动。
const int START_MOVE_ERROR_MIN = 2;

// ============================================================================
// PID & accel limiting (UPDATED)
// ============================================================================
// EN PID parameters (tune). Higher Kp = stronger response, Ki reduces steady error,
//    Kd helps damp overshoot. Filter noise is lower, so small Kd can work.
// CN PID 参数（需调参）。Kp 越大响应越强，Ki 消除静差，Kd 抑制过冲。
//    因为滤波更稳了，所以 Kd 可以用但不要过大。
float Kp = 5.0f;
float Ki = 0.08f;
float Kd = 0.06f;

float pidIntegral = 0.0f;
float pidPrevError = 0.0f;

// EN Enable PID only in near zone (remaining distance <= window). 
// CN 仅在近区启用 PID（剩余距离小于等于窗口）。
const float PID_ENABLE_WINDOW = 15.0f;

// EN Acceleration limiting (PWM ramp rate), avoids sudden changes / shocks. 
// CN 加速度（PWM 斜率）限制，避免突变导致冲击/抖动。
const float ACCEL_PWM_PER_S = 900.0f;
float pwmCmd = 0.0f;

// EN Integral clamp for anti-windup. 
// CN 积分限幅，防止 windup（积分饱和导致严重过冲）。
const float I_CLAMP = 1200.0f;

// ============================================================================
// Homing: trend + stall
// ============================================================================
// EN Homing uses a sliding window range test: if range is small for long enough,
//    we assume actuator hit mechanical stop (stall).
// CN 回零采用“趋势+停滞”判据：窗口范围持续很小，则认为顶到机械端（停滞）。
const unsigned long HOME_WINDOW_MS = 1200;
const unsigned long HOME_SAMPLE_MS = 50;
const int HOME_WINDOW_RANGE_MAX = 2;
const unsigned long HOME_STALL_CONFIRM_MS = 2000;

// EN Extra retract time after stall confirmed, to ensure firmly seated at end. 
// CN 确认停滞后额外继续回缩一小段时间，确保顶牢端点。
const unsigned long HOME_EXTRA_MS = 200;

// -------------------- Direction mapping --------------------
// EN True means: driving RPWM direction makes sensor increase => RPWM = EXTEND.
// CN true 表示：RPWM 方向会让传感器读数增大 => RPWM 即为伸出方向（EXTEND）。
bool rpwmIsExtend = true;

// -------------------- State machine --------------------
// EN MOVING: closed-loop control enabled; HOLD_DONE: motor stopped, only report. 
// CN MOVING：闭环控制；HOLD_DONE：停止保持，只上报不驱动电机。
enum RunState { MOVING, HOLD_DONE };
RunState state = HOLD_DONE;

// EN Flags/timers for DONE detection. 
// CN 完成判定相关的标志与计时器。
bool donePrinted = false;
unsigned long stableStartMs = 0;
int stableRefPos = 0;

// ============================================================================
// Motor control
// ============================================================================
// EN Stop both PWM channels (coast/brake depends on driver wiring). 
// CN 两路 PWM 都置 0，停止电机（实际为滑行或制动取决于接线/驱动行为）。
void stopMotor() {
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
}

// EN Drive only RPWM (one direction). Ensure other channel is 0 to avoid shoot-through. 
// CN 只输出 RPWM（一个方向），另一通道清零避免上下桥同时导通风险。
void driveRawRPWM(int pwm) {
  pwm = constrain(pwm, 0, PWM_MAX);
  analogWrite(LPWM_PIN, 0);
  analogWrite(RPWM_PIN, pwm);
}

// EN Drive only LPWM (opposite direction). 
// CN 只输出 LPWM（相反方向）。
void driveRawLPWM(int pwm) {
  pwm = constrain(pwm, 0, PWM_MAX);
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, pwm);
}

// EN Drive EXTEND direction using the discovered mapping rpwmIsExtend. 
// CN 根据 rpwmIsExtend 的映射驱动“伸出”方向。
void driveExtendPwm(int pwm) {
  pwm = constrain(pwm, 0, PWM_MAX);
  if (pwm == 0) { stopMotor(); return; }
  if (rpwmIsExtend) driveRawRPWM(pwm);
  else driveRawLPWM(pwm);
}

// EN Drive RETRACT direction using the discovered mapping rpwmIsExtend. 
// CN 根据 rpwmIsExtend 的映射驱动“回缩”方向。
void driveRetractPwm(int pwm) {
  pwm = constrain(pwm, 0, PWM_MAX);
  if (pwm == 0) { stopMotor(); return; }
  if (rpwmIsExtend) driveRawLPWM(pwm);
  else driveRawRPWM(pwm);
}

// EN Signed PWM command: >0 extend, <0 retract. 
// CN 带符号 PWM：>0 伸出，<0 回缩。
void driveSigned(float signedPwm) {
  int pwm = (int)fabs(signedPwm);
  if (pwm <= 0) { stopMotor(); return; }
  pwm = constrain(pwm, 0, PWM_MAX);

  if (signedPwm > 0) driveExtendPwm(pwm);
  else driveRetractPwm(pwm);
}

// EN Breakaway MIN_PWM assist: only when we truly need movement and command is too small.
//    Avoid forcing MIN_PWM when error is near deadband (to prevent oscillation).
// CN MIN_PWM 起动辅助：仅在确实需要移动但输出过小的时候抬升。
//    误差接近死区时不强制 MIN_PWM，避免到点抖动/来回冲。
int applyBreakawayMinPwm(int pwmMag, int errAbsInt)
{
  if (pwmMag <= 0) return 0;

  // EN Near deadband: do not force MIN_PWM. 
  // CN 接近死区：不要强制 MIN_PWM。
  if (errAbsInt <= (POS_DEADBAND_INT + 1)) {
    return pwmMag;
  }

  // EN If too small, raise to MIN_PWM to overcome static friction. 
  // CN 如果命令过小，抬到 MIN_PWM 克服静摩擦。
  if (pwmMag < MIN_PWM) return MIN_PWM;

  return pwmMag;
}

// ============================================================================
// Serial input
// ============================================================================
// EN Reset PID states and completion detection when a new target is set. 
// CN 设置新目标时重置 PID 状态与完成检测（避免旧状态影响新目标）。
void resetControlForNewTarget()
{
  pidIntegral = 0.0f;
  pidPrevError = 0.0f;
  pwmCmd = 0.0f;

  donePrinted = false;
  stableStartMs = 0;
  stableRefPos = 0;
}

// EN Enter MOVING with a new target: reset controller and announce. 
// CN 进入 MOVING 并设置新目标：重置控制器并串口提示。
void enterMovingForNewTarget(int newTarget)
{
  target1000 = newTarget;
  resetControlForNewTarget();
  state = MOVING;

  Serial.print("A1 TARGET_SET ");
  Serial.println(target1000);
}

// EN Parse and validate a target line (must be integer 0..1000, no junk). 
// CN 解析并校验目标输入行（必须是 0..1000 的整数，不能带杂字符）。
void processTargetLine(const char* line)
{
  char* endPtr;
  long v = strtol(line, &endPtr, 10);

  // EN Skip trailing spaces/tabs. 
  // CN 跳过末尾空格/制表符。
  while (*endPtr == ' ' || *endPtr == '\t') endPtr++;

  // EN Reject empty or invalid parse or out-of-range. 
  // CN 空串/解析失败/超范围则拒绝。
  if (line[0] == '\0' || *endPtr != '\0' || v < 0 || v > 1000) {
    Serial.println("A1 INVALID_TARGET");
    return;
  }

  int newTarget = (int)v;
  int err = abs(newTarget - pos1000);

  // EN If holding done, only restart movement when difference is significant. 
  // CN 若处于完成保持，仅当新目标差异足够大才重新运动。
  if (state == HOLD_DONE) {
    if (err >= START_MOVE_ERROR_MIN) enterMovingForNewTarget(newTarget);
    else {
      target1000 = newTarget;
      Serial.print("A1 TARGET_SET ");
      Serial.println(target1000);
    }
  } else {
    // EN While moving: update target immediately and reset controller to avoid legacy bias. 
    // CN 运动中更新目标：立即更新并重置控制器，避免旧积分/稳定计时干扰。
    target1000 = newTarget;
    resetControlForNewTarget();
    Serial.print("A1 TARGET_SET ");
    Serial.println(target1000);
  }
}

// EN Serial line reader: accumulate until CR/LF, then process one line. 
// CN 串口行读取：累积字符直到 CR/LF，然后处理整行。
void handleSerialInput()
{
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();

    // EN End-of-line: validate and consume buffer. 
    // CN 行结束：校验并处理缓冲区内容。
    if (ch == '\r' || ch == '\n') {
      if (serialLineOverflow) {
        Serial.println("A1 INVALID_TARGET");
      } else {
        serialBuffer[serialBufferIndex] = '\0';
        processTargetLine(serialBuffer);
      }
      serialBufferIndex = 0;
      serialLineOverflow = false;
      continue;
    }

    // EN If overflow already happened, ignore until newline. 
    // CN 一旦溢出，直到遇到换行前都丢弃字符。
    if (serialLineOverflow) continue;

    // EN Append character if space remains; otherwise flag overflow. 
    // CN 有空间则追加字符，否则标记溢出。
    if (serialBufferIndex < SERIAL_BUFFER_LEN - 1) {
      serialBuffer[serialBufferIndex++] = ch;
    } else {
      serialLineOverflow = true;
    }
  }
}

// ============================================================================
// Init Step 1: direction confirm (2 cycles)
// ============================================================================
// EN Purpose:
//    Because wiring or mechanics may invert direction, we empirically detect which
//    PWM channel causes sensor to increase. That channel is labeled EXTEND.
// CN 目的：
//    由于接线/机构可能导致方向反转，所以通过实验检测“哪个通道使传感器增大”，
//    该通道就定义为 EXTEND（伸出）。
void initDirectionConfirmReciprocate2()
{
  Serial.println("A1 INIT_STEP1_DIRCONF_START");

  // EN Leg duration: long enough to see clear delta. 
  // CN 单段运动时间：需要足够长，保证传感器变化明显。
  const unsigned long LEG_MS = 4500;

  // EN Test PWM uses PWM_INIT speed. 
  // CN 测试 PWM 使用 PWM_INIT 的速度。
  const int TEST_PWM = PWM_INIT;

  // EN Accumulate deltas over 2 reciprocations to reduce noise. 
  // CN 累计 2 个往返的变化量，以降低噪声与偶然误判。
  long dR_total = 0;
  long dL_total = 0;

  for (int cycle = 0; cycle < 2; cycle++) {
    // ---------------- RPWM leg ----------------
    // EN Measure sensor before and after driving RPWM. 
    // CN 驱动 RPWM 前后分别测量传感器，记录变化量。
    int s0 = readScaled1000ModeVote();
    unsigned long t0 = millis();
    while (millis() - t0 < LEG_MS) {
      driveRawRPWM(TEST_PWM);
      handleSerialInput(); // EN Keep serial responsive // CN 保持串口响应
      delay(10);
    }
    stopMotor();
    delay(250); // EN Let system settle // CN 稳定一下
    int s1 = readScaled1000ModeVote();
    dR_total += (long)(s1 - s0);

    // ---------------- LPWM leg ----------------
    int s2 = readScaled1000ModeVote();
    t0 = millis();
    while (millis() - t0 < LEG_MS) {
      driveRawLPWM(TEST_PWM);
      handleSerialInput();
      delay(10);
    }
    stopMotor();
    delay(250);
    int s3 = readScaled1000ModeVote();
    dL_total += (long)(s3 - s2);
  }

  Serial.print("A1 DIRCONF dR_total=");
  Serial.print(dR_total);
  Serial.print(" dL_total=");
  Serial.println(dL_total);

  // EN Decide: channel producing positive delta is EXTEND (heuristics for edge cases). 
  // CN 判定：产生正变化量的通道为 EXTEND（包含一些边界情况的兜底逻辑）。
  if (dR_total >= dL_total) {
    rpwmIsExtend = (dR_total > 0);
    if (dR_total == 0 && dL_total != 0) rpwmIsExtend = (dL_total < 0);
    if (dR_total == 0 && dL_total == 0) rpwmIsExtend = true;
  } else {
    rpwmIsExtend = false;
    if (dL_total <= 0 && dR_total > 0) rpwmIsExtend = true;
  }

  Serial.print("A1 RPWM_IS_EXTEND ");
  Serial.println(rpwmIsExtend ? "1" : "0");
  Serial.println("A1 INIT_STEP1_DIRCONF_DONE");
}

// ============================================================================
// Init Step 2: homing retract using "trend + stall" (origin more robust)
// ============================================================================
// EN Helper: median of 5 values, robust against a single outlier.
// CN 工具函数：取 5 个值的中位数，对单个异常值更鲁棒。
int medianOf5(int a[5]) {
  // EN In-place insertion sort for 5 elements, then pick middle. 
  // CN 对 5 个元素做插入排序，然后取中间值。
  for (int i = 1; i < 5; i++) {
    int key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) { a[j + 1] = a[j]; j--; }
    a[j + 1] = key;
  }
  return a[2];
}

// EN Homing procedure:
//    1) Retract at PWM_INIT
//    2) Every HOME_SAMPLE_MS read sensor (filtered)
//    3) Maintain a window (HOME_WINDOW_MS) min/max; if range <= threshold for
//       HOME_STALL_CONFIRM_MS => consider stalled at mechanical end
//    4) Extra retract HOME_EXTRA_MS, stop, then set zeroScaled using median of stable samples
// CN 回零流程：
//    1) 以 PWM_INIT 持续回缩
//    2) 每隔 HOME_SAMPLE_MS 读取一次传感器（已滤波）
//    3) 用 HOME_WINDOW_MS 时间窗统计 min/max；若 range <= 阈值并持续
//       HOME_STALL_CONFIRM_MS，则认为已顶到机械端
//    4) 额外回缩 HOME_EXTRA_MS 后停止，再用末端稳定样本中位数设 zeroScaled
void initHomeToMinTrendStall()
{
  Serial.println("A1 INIT_STEP2_HOME_START");

  unsigned long lastSampleMs = 0;
  unsigned long winStartMs = millis();

  // EN Window min/max for range test. 
  // CN 用于范围检测的窗口最小/最大值。
  int winMin = 9999;
  int winMax = -9999;

  // EN When range is small, start stall timer; if sustained, homing done. 
  // CN 当 range 很小时启动停滞计时器，持续足够久则回零完成。
  unsigned long stallStartMs = 0;

  // EN Track global minimum seen (not used for final zero, but useful diagnostic). 
  // CN 记录全程最小值（本版本不直接用它作为零点，但可作参考）。
  int minSeen = readScaled1000ModeVote();
  winMin = minSeen;
  winMax = minSeen;

  while (true) {
    unsigned long now = millis();

    // EN Keep retracting during homing. 
    // CN 回零阶段持续回缩。
    driveRetractPwm(PWM_INIT);
    handleSerialInput();

    // EN Sample at fixed interval. 
    // CN 按固定间隔采样。
    if (now - lastSampleMs < HOME_SAMPLE_MS) continue;
    lastSampleMs = now;

    // EN Read filtered sensor value. 
    // CN 读取滤波后的传感器值。
    int v = readScaled1000ModeVote();

    // EN Update global minimum and window min/max. 
    // CN 更新全程最小值以及窗口 min/max。
    if (v < minSeen) minSeen = v;
    if (v < winMin) winMin = v;
    if (v > winMax) winMax = v;

    // EN Every HOME_WINDOW_MS evaluate range and confirm stall. 
    // CN 每隔 HOME_WINDOW_MS 评估一次 range 并判断是否停滞。
    if (now - winStartMs >= HOME_WINDOW_MS) {
      int range = winMax - winMin;

      // EN If range small enough, start/continue stall confirm timer. 
      // CN 如果 range 足够小，启动/继续停滞确认计时。
      if (range <= HOME_WINDOW_RANGE_MAX) {
        if (stallStartMs == 0) stallStartMs = now;
        if (now - stallStartMs >= HOME_STALL_CONFIRM_MS) break;
      } else {
        // EN Range too large => still moving; reset stall timer. 
        // CN range 太大 => 仍在运动；重置停滞计时器。
        stallStartMs = 0;
      }

      // EN Reset window with current value as seed. 
      // CN 重置窗口，并用当前值作为新窗口的初值。
      winStartMs = now;
      winMin = v;
      winMax = v;
    }
  }

  // EN Extra retract time ensures hard stop seating. 
  // CN 额外回缩一小段时间，确保顶到位。
  unsigned long tHold = millis();
  while (millis() - tHold < HOME_EXTRA_MS) {
    driveRetractPwm(PWM_INIT);
    handleSerialInput();
    delay(10);
  }

  stopMotor();
  delay(250);

  // EN Key: set origin using median of stable end samples (more robust than global min). 
  // CN 关键：用末端稳定采样的中位数设零点（比全程最小值更抗偶发低值）。
  int z[5];
  for (int i = 0; i < 5; i++) {
    z[i] = readScaled1000ModeVote();
    delay(20);
  }
  zeroScaled = medianOf5(z);

  // EN Update current position after zeroing. 
  // CN 设零后刷新当前位置。
  pos1000 = readPos1000();

  Serial.print("A1 ZERO_SCALED ");
  Serial.println(zeroScaled);
  Serial.println("A1 INIT_STEP2_HOME_DONE");
}

// ============================================================================
// Control
// ============================================================================
// EN PID compute with integral clamp and derivative on error. Output clipped to ±PWM_MAX.
// CN PID 计算：积分限幅 + 误差微分，输出限制在 ±PWM_MAX。
float pidCompute(float error, float dt)
{
  pidIntegral += error * dt;
  if (pidIntegral >  I_CLAMP) pidIntegral =  I_CLAMP;
  if (pidIntegral < -I_CLAMP) pidIntegral = -I_CLAMP;

  float deriv = (error - pidPrevError) / dt;
  pidPrevError = error;

  float out = Kp * error + Ki * pidIntegral + Kd * deriv;
  if (out >  PWM_MAX) out =  PWM_MAX;
  if (out < -PWM_MAX) out = -PWM_MAX;
  return out;
}

// EN DONE logic:
//    - If |error| > deadband => reset stability timer.
//    - Else start timer; require position not drifting beyond STABLE_POS_TOL.
//    - If stable for STABLE_TIME_MS => stop motor and enter HOLD_DONE.
// CN DONE 逻辑：
//    - 若 |误差| > 死区 => 重置稳定计时。
//    - 否则开始计时；要求位置漂移不超过 STABLE_POS_TOL。
//    - 稳定满 STABLE_TIME_MS => 停止电机并进入 HOLD_DONE。
void updateCompletionAndMaybeHold(unsigned long nowMs)
{
  int errAbs = abs(target1000 - pos1000);

  if (errAbs > POS_DEADBAND_INT) {
    stableStartMs = 0;
    stableRefPos = pos1000;
    donePrinted = false;
    return;
  }

  if (stableStartMs == 0) {
    stableStartMs = nowMs;
    stableRefPos = pos1000;
    return;
  }

  if (abs(pos1000 - stableRefPos) > STABLE_POS_TOL) {
    stableStartMs = nowMs;
    stableRefPos = pos1000;
    return;
  }

  if (nowMs - stableStartMs >= STABLE_TIME_MS) {
    stopMotor();
    pwmCmd = 0.0f;

    if (!donePrinted) {
      Serial.println("A1 DONE");
      donePrinted = true;
    }

    state = HOLD_DONE;
  }
}

// EN One control step while MOVING.
//    - Read position
//    - Decide desired PWM using 3-zone strategy (far/mid/near PID)
//    - Apply accel limiting to pwmCmd, then drive motor
// CN MOVING 状态下的一次控制步：
//    - 读取位置
//    - 使用三段策略（远/中/近PID）得到期望 PWM
//    - 通过加速度限制更新 pwmCmd，并驱动电机
void controlStepMoving(float dt, unsigned long nowMs)
{
  pos1000 = readPos1000();

  float error = (float)target1000 - (float)pos1000;
  float errAbsF = fabs(error);
  int errAbsI = abs(target1000 - pos1000);

  // EN Deadband handling: ramp down pwmCmd smoothly; check completion stability.
// CN 死区处理：平滑降速到 0，同时进行完成稳定判定。
  if (errAbsF <= (float)POS_DEADBAND_INT) {
    float maxStep = ACCEL_PWM_PER_S * dt;

    // EN Ramp pwmCmd toward zero with accel limit. 
    // CN 以加速度限制将 pwmCmd 缓慢收敛到 0。
    if (fabs(pwmCmd) <= maxStep) pwmCmd = 0.0f;
    else pwmCmd += (pwmCmd > 0 ? -maxStep : maxStep);

    driveSigned(pwmCmd);

    updateCompletionAndMaybeHold(nowMs);
    return;
  }

  // EN If outside deadband, movement is ongoing: reset DONE stability tracking. 
  // CN 若不在死区内，说明仍在运动：重置 DONE 稳定计时相关状态。
  stableStartMs = 0;
  stableRefPos = pos1000;
  donePrinted = false;

  float desired = 0.0f;

  // EN 3-zone control strategy:
  //    Far: full speed; Mid: fixed speed; Near: PID.
  // CN 三段控制策略：
  //    远：全速；中：定速；近：PID 微调。
  if (errAbsF > 80.0f) {
    desired = (error > 0) ? (float)PWM_MAX : -(float)PWM_MAX;
  } else if (errAbsF > PID_ENABLE_WINDOW) {
    // EN Mid distance: reduced fixed PWM to approach more gently. 
    // CN 中距离：降低定速 PWM，使接近更柔和。
    const float MID_PWM = 180.0f;
    desired = (error > 0) ? MID_PWM : -MID_PWM;
  } else {
    // EN Near zone: PID for precise convergence. 
    // CN 近距离：用 PID 精准逼近。
    desired = pidCompute(error, dt);

    // EN Apply MIN_PWM breakaway only when error is not too small. 
    // CN 仅在误差不太小时使用 MIN_PWM 起动辅助。
    int mag = (int)fabs(desired);
    mag = applyBreakawayMinPwm(mag, errAbsI);
    desired = (desired >= 0) ? (float)mag : -(float)mag;
  }

  // EN Accel limiting: limit change rate of pwmCmd to avoid jerk. 
  // CN 加速度限制：限制 pwmCmd 变化率，避免突变冲击。
  float maxStep = ACCEL_PWM_PER_S * dt;
  float delta = desired - pwmCmd;
  if (delta >  maxStep) delta =  maxStep;
  if (delta < -maxStep) delta = -maxStep;
  pwmCmd += delta;

  driveSigned(pwmCmd);
}

// ============================================================================
// Arduino setup/loop
// ============================================================================
// EN setup(): initialize serial, pins, then run initialization steps:
//    1) Direction confirm
//    2) Homing to mechanical minimum
//    Finally enter HOLD_DONE at target=0.
// CN setup()：初始化串口与引脚，然后执行初始化流程：
//    1) 方向确认
//    2) 回零到机械最小端
//    最后以 target=0 进入 HOLD_DONE。
void setup()
{
  Serial.begin(SERIAL_BAUD);
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);

  stopMotor();
  delay(300);

  // EN Initialize filter tie-break baseline with a single raw reading. 
  // CN 用一次原始读取初始化滤波 tie-break 基准。
  lastFilteredScaled = rawToScaled1000(analogRead(A1S_PIN));

  Serial.println("A1 READY");
  Serial.println("A1 INIT_START");

  initDirectionConfirmReciprocate2();
  initHomeToMinTrendStall();

  Serial.println("A1 INIT_DONE");

  // EN After init, refresh position and set default target. 
  // CN 初始化结束后刷新位置并设置默认目标。
  pos1000 = readPos1000();
  target1000 = 0;

  // EN Reset control variables and stop motor. 
  // CN 重置控制变量并停止电机。
  resetControlForNewTarget();
  stopMotor();

  // EN Start in HOLD_DONE, and print target. 
  // CN 以 HOLD_DONE 开始，并输出当前目标。
  state = HOLD_DONE;
  donePrinted = true;
  stableStartMs = 0;

  Serial.print("A1 TARGET_SET ");
  Serial.println(target1000);

  lastControlMs = millis();
  lastReportMs  = millis();
}

// EN loop(): handle serial; update control if MOVING; otherwise HOLD_DONE.
//    Report position periodically depending on state.
// CN loop()：处理串口；MOVING 则进行控制更新；HOLD_DONE 则保持停止。
//    按状态不同周期输出位置。
void loop()
{
  handleSerialInput();
  unsigned long now = millis();

  if (state == MOVING) {
    // EN Run control step at CONTROL_DT_MS period. 
    // CN 按 CONTROL_DT_MS 周期运行控制步。
    if (now - lastControlMs >= CONTROL_DT_MS) {
      float dt = (now - lastControlMs) / 1000.0f;
      lastControlMs = now;
      controlStepMoving(dt, now);
    }
  } else {
    // EN HOLD_DONE: only read & report; do not move based on sensor. 
    // CN HOLD_DONE：仅读取并上报，不基于传感器做任何运动控制。
    pos1000 = readPos1000();
    stopMotor();
    pwmCmd = 0.0f;
  }

  // EN Telemetry output period depends on state. 
  // CN 上报周期随状态变化。
  unsigned long reportPeriod = (state == MOVING) ? REPORT_DT_MS_MOVING : REPORT_DT_MS_HOLD;
  if (now - lastReportMs >= reportPeriod) {
    lastReportMs = now;
    Serial.print("A1 POS ");
    Serial.println(pos1000);
  }
}