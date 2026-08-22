/*
  Linear actuator test controller for Arduino Mega.

  说明 / Description:
  - This file preserves the JSON telemetry and serial command parsing structure.
  - The motion logic is reworked into three stages according to the requested test flow.
  - The original communication contract remains compatible with the existing JSON style:
      T,<a1_target>,<a2_target>,<a3_target>,<a4_target>

  这是一个测试用控制器文件。
  - 保留了原有的 JSON 遥测格式以及串口指令解析结构。
  - 运动逻辑按照你要求改写为三个阶段。
  - 仍然使用原有的指令格式：
      T,<a1_target>,<a2_target>,<a3_target>,<a4_target>
*/

#include <Arduino_JSON.h>
#include <stdlib.h>

// ============================================================================
// 1. 基础硬件映射 / Basic hardware mapping
// ============================================================================
const int ACTUATOR_COUNT = 4;

// 四路执行器必须按相同的数组下标一一对应：
// actuator 0: sensor A0, RPWM 2, LPWM 3
// actuator 1: sensor A1, RPWM 4, LPWM 5
// actuator 2: sensor A2, RPWM 6, LPWM 7
// actuator 3: sensor A3, RPWM 8, LPWM 9
const int SENSOR_PINS[ACTUATOR_COUNT] = {A0, A1, A2, A3};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool ACTUATOR_ENABLED[ACTUATOR_COUNT] = {true, true, true, true};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};
bool sensorConnected[ACTUATOR_COUNT] = {true, true, true, true};

// ============================================================================
// 2. 控制参数 / Control parameters
// ============================================================================
const int DEFAULT_TARGET_MM = 50;
const int TARGET_DEADBAND_MM = 2;
const int PID_START_ERROR_MM = 10;
const int DRIVE_PWM = 200;
const int MIN_PID_PWM = 20;
const int CALIBRATION_PWM = 200;
const unsigned long CALIBRATION_DURATION_MS = 2000;
const unsigned long CALIBRATION_DELAY_MS = 200;
const int CALIBRATION_SENSOR_DELTA_THRESHOLD = 2;
const unsigned long STALL_TIMEOUT_MS = 3000;
const int STALL_RAW_DELTA_THRESHOLD = 2;
const bool STOP_ON_STALL = false;
const int MODE_FILTER_WINDOW = 5;
const float PID_KP = 12.0f;
const float PID_KI = 0.4f;
const float PID_KD = 3.0f;

const unsigned long SAMPLE_INTERVAL_MS = 100;
const unsigned long CALIBRATION_SAMPLE_INTERVAL_MS = 20;
const unsigned long TELEMETRY_INTERVAL_MS = 1000;

const int SERIAL_BUFFER_LEN = 256;
char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;
bool serialLineOverflow = false;

// ============================================================================
// 3. 传感器与位置映射 / Sensor and position mapping
// ============================================================================
// 这里先使用一组固定的初始标定预设值。
// This uses an initial preset calibration map.
const int CAL_RAW_AT_0 = 3;
const int CAL_RAW_AT_800 = 812;
const int CAL_RAW_SPAN = 809;
const bool CAL_RPWM_IS_EXTEND = true;
const bool CAL_ZERO_IS_LOW_SENSOR = true;
const bool CAL_EEPROM_SAVED = true;

int currentMm[ACTUATOR_COUNT] = {0, 0, 0, 0};
int currentRaw[ACTUATOR_COUNT] = {0, 0, 0, 0};
int targetMm[ACTUATOR_COUNT] = {DEFAULT_TARGET_MM, DEFAULT_TARGET_MM, DEFAULT_TARGET_MM, DEFAULT_TARGET_MM};
int lastProgressRaw[ACTUATOR_COUNT] = {0, 0, 0, 0};
unsigned long lastProgressMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
bool stallWarningReported[ACTUATOR_COUNT] = {false, false, false, false};
int pidIntegral[ACTUATOR_COUNT] = {0, 0, 0, 0};
int pidPreviousError[ACTUATOR_COUNT] = {0, 0, 0, 0};
int sensorHistory[ACTUATOR_COUNT][MODE_FILTER_WINDOW];
int sensorHistoryIndex[ACTUATOR_COUNT] = {0, 0, 0, 0};
int sensorHistoryCount[ACTUATOR_COUNT] = {0, 0, 0, 0};

// ============================================================================
// 4. 运动方向校准结果 / Calibration result
// ============================================================================
// 这部分用于记录：哪一个 PWM 方向对应“伸出”。
// This stores which PWM direction corresponds to extension.
bool rpwmIsExtend[ACTUATOR_COUNT] = {false, false, false, false};
bool actuatorMotionActive[ACTUATOR_COUNT] = {false, false, false, false};
bool calibrationComplete = false;
bool calibrationSensorFailure = false;
bool targetsActive = false;
bool initialMoveStarted = false;

enum CalibrationPhase
{
  CAL_IDLE,
  CAL_DRIVE_RPWM,
  CAL_SETTLE_AFTER_RPWM,
  CAL_DRIVE_LPWM,
  CAL_SETTLE_AFTER_LPWM,
  CAL_DONE
};

CalibrationPhase calibrationPhase[ACTUATOR_COUNT] = {CAL_IDLE, CAL_IDLE, CAL_IDLE, CAL_IDLE};
unsigned long calibrationPhaseStartMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
unsigned long calibrationLastSampleMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationStartSample[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationLastSample[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationRpwmTrend[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationLpwmTrend[ACTUATOR_COUNT] = {0, 0, 0, 0};

unsigned long lastSampleMs = 0;
unsigned long lastTelemetryMs = 0;

const bool DEBUG_SERIAL = true;

// ============================================================================
// 5. 调试辅助函数 / Debug helpers
// ============================================================================
void debugLog(const char *message)
{
  if (!DEBUG_SERIAL)
  {
    return;
  }

  Serial.print("DBG ");
  Serial.println(message);
}

void debugTargets(const char *label)
{
  if (!DEBUG_SERIAL)
  {
    return;
  }

  Serial.print("DBG ");
  Serial.print(label);
  Serial.print(" targets=[");

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    Serial.print(targetMm[i]);
    if (i < ACTUATOR_COUNT - 1)
    {
      Serial.print(",");
    }
  }

  Serial.println("]");
}

// ============================================================================
// 6. 基础驱动与读取函数 / Basic drive and sensor functions
// ============================================================================
bool isActuatorConfigured(int actuatorIndex)
{
  return ACTUATOR_ENABLED[actuatorIndex] && sensorConnected[actuatorIndex];
}

void stopMotor(int actuatorIndex)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    return;
  }

  analogWrite(RPWM_PINS[actuatorIndex], 0);
  analogWrite(LPWM_PINS[actuatorIndex], 0);
}

void stopAllMotors()
{
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    stopMotor(i);
  }
}

// ============================================================================
// 6.1 传感器采样与滤波 / Sensor sampling and filtering
// ============================================================================
// 这里使用“最近 5 个采样值的中值滤波”来抑制噪声。
// This uses a median filter over the most recent 5 samples to suppress sensor noise.
int readModeFilteredSensor(int actuatorIndex)
{
  int raw = analogRead(SENSOR_PINS[actuatorIndex]);

  int *history = sensorHistory[actuatorIndex];
  int &historyIndex = sensorHistoryIndex[actuatorIndex];
  int &historyCount = sensorHistoryCount[actuatorIndex];

  history[historyIndex] = raw;
  historyIndex = (historyIndex + 1) % MODE_FILTER_WINDOW;
  if (historyCount < MODE_FILTER_WINDOW)
  {
    historyCount++;
  }

  int windowSize = historyCount;
  int values[MODE_FILTER_WINDOW];
  for (int i = 0; i < windowSize; i++)
  {
    int slot = (historyIndex + MODE_FILTER_WINDOW - 1 - i) % MODE_FILTER_WINDOW;
    values[i] = history[slot];
  }

  for (int i = 1; i < windowSize; i++)
  {
    int key = values[i];
    int j = i - 1;
    while (j >= 0 && values[j] > key)
    {
      values[j + 1] = values[j];
      j--;
    }
    values[j + 1] = key;
  }

  return values[windowSize / 2];
}

void driveActuator(int actuatorIndex, bool useRPWM, int pwm)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    return;
  }

  if (useRPWM)
  {
    analogWrite(LPWM_PINS[actuatorIndex], 0);
    analogWrite(RPWM_PINS[actuatorIndex], pwm);
    return;
  }

  analogWrite(RPWM_PINS[actuatorIndex], 0);
  analogWrite(LPWM_PINS[actuatorIndex], pwm);
}

// ============================================================================
// 6.2 PID 控制 / PID control
// ============================================================================
// 远离目标时保持较高 PWM，接近目标时切换为 PID，减少过冲和震荡。
// Use a stronger PWM when far from target, and switch to PID near the target to reduce overshoot and oscillation.
void resetPidState(int actuatorIndex)
{
  pidIntegral[actuatorIndex] = 0;
  pidPreviousError[actuatorIndex] = 0;
}

void applyActuatorControl(int actuatorIndex, int error)
{
  if (abs(error) <= TARGET_DEADBAND_MM)
  {
    stopMotor(actuatorIndex);
    resetPidState(actuatorIndex);
    return;
  }

  bool shouldExtend = (error > 0);
  bool useRPWM = rpwmIsExtend[actuatorIndex];
  if (INVERT_DIRECTION[actuatorIndex])
  {
    useRPWM = !useRPWM;
  }
  if (!shouldExtend)
  {
    useRPWM = !useRPWM;
  }

  int pwm = DRIVE_PWM;
  if (abs(error) <= PID_START_ERROR_MM)
  {
    pidIntegral[actuatorIndex] += error;
    pidIntegral[actuatorIndex] = constrain(pidIntegral[actuatorIndex], -50, 50);

    int derivative = error - pidPreviousError[actuatorIndex];
    float pidOutput = (PID_KP * (float)error) + (PID_KI * (float)pidIntegral[actuatorIndex]) + (PID_KD * (float)derivative);
    pidPreviousError[actuatorIndex] = error;

    pwm = (int)constrain(abs(pidOutput), MIN_PID_PWM, DRIVE_PWM);
  }
  else
  {
    resetPidState(actuatorIndex);
  }

  driveActuator(actuatorIndex, useRPWM, pwm);
}

int rawToPositionMm(int raw)
{
  if (raw <= CAL_RAW_AT_0)
  {
    return 0;
  }

  if (raw >= CAL_RAW_AT_800)
  {
    return 800;
  }

  long span = (long)CAL_RAW_AT_800 - (long)CAL_RAW_AT_0;
  if (span <= 0)
  {
    return 0;
  }

  long pos = ((long)(raw - CAL_RAW_AT_0) * 800L) / span;
  if (pos < 0)
  {
    pos = 0;
  }
  if (pos > 800)
  {
    pos = 800;
  }

  return (int)pos;
}

void updateCurrentMmFromSensors()
{
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!isActuatorConfigured(i))
    {
      continue;
    }

    int raw = readModeFilteredSensor(i);
    currentRaw[i] = raw;
    currentMm[i] = rawToPositionMm(raw);
  }
}

// ============================================================================
// 7. 阶段 1：初始方向校准 / Stage 1: initial direction calibration
// ============================================================================
// 这一段负责在启动时判断：哪一个 PWM 会使传感器值增大。
// This block checks which PWM direction causes the sensor reading to increase.
void finishActuatorCalibration(int actuatorIndex)
{
  int rpwmTrend = calibrationRpwmTrend[actuatorIndex];
  int lpwmTrend = calibrationLpwmTrend[actuatorIndex];
  bool sensorResponds = true;
  bool extendsWithRPWM = CAL_RPWM_IS_EXTEND;

  if (abs(rpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    bool rpwmIncreasesSensor = (rpwmTrend > 0);
    extendsWithRPWM = CAL_ZERO_IS_LOW_SENSOR ? rpwmIncreasesSensor : !rpwmIncreasesSensor;
  }
  else if (abs(lpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    bool lpwmIncreasesSensor = (lpwmTrend > 0);
    bool lpwmIsExtend = CAL_ZERO_IS_LOW_SENSOR ? lpwmIncreasesSensor : !lpwmIncreasesSensor;
    extendsWithRPWM = !lpwmIsExtend;
  }
  else
  {
    sensorResponds = false;
  }

  rpwmIsExtend[actuatorIndex] = extendsWithRPWM;
  calibrationPhase[actuatorIndex] = CAL_DONE;

  if (!sensorResponds)
  {
    sensorConnected[actuatorIndex] = false;
    calibrationSensorFailure = true;
    Serial.print("DBG sensor ");
    Serial.print(actuatorIndex);
    Serial.println(" not responding during calibration -> disabled");
    return;
  }

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG cal actuator ");
    Serial.print(actuatorIndex);
    Serial.print(" rpwm_trend=");
    Serial.print(rpwmTrend);
    Serial.print(" lpwm_trend=");
    Serial.print(lpwmTrend);
    Serial.print(" rpwm_is_extend=");
    Serial.println(extendsWithRPWM ? 1 : 0);
  }
}

void startActuatorDirectionCalibration()
{
  debugLog("starting concurrent direction calibration");
  stopAllMotors();
  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!isActuatorConfigured(i))
    {
      calibrationPhase[i] = CAL_DONE;
      continue;
    }

    int sample = readModeFilteredSensor(i);
    calibrationStartSample[i] = sample;
    calibrationLastSample[i] = sample;
    calibrationPhaseStartMs[i] = now;
    calibrationLastSampleMs[i] = now;
    calibrationPhase[i] = CAL_DRIVE_RPWM;
    driveActuator(i, true, CALIBRATION_PWM);
  }
}

void updateActuatorDirectionCalibration()
{
  if (calibrationComplete)
  {
    return;
  }

  unsigned long now = millis();
  bool allDone = true;

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    CalibrationPhase phase = calibrationPhase[i];
    if (phase == CAL_DONE)
    {
      continue;
    }

    allDone = false;
    if (phase == CAL_DRIVE_RPWM || phase == CAL_DRIVE_LPWM)
    {
      if (now - calibrationLastSampleMs[i] >= CALIBRATION_SAMPLE_INTERVAL_MS)
      {
        calibrationLastSampleMs[i] = now;
        calibrationLastSample[i] = readModeFilteredSensor(i);
      }
    }

    if (phase == CAL_DRIVE_RPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DURATION_MS)
    {
      stopMotor(i);
      calibrationLastSample[i] = readModeFilteredSensor(i);
      calibrationRpwmTrend[i] = calibrationLastSample[i] - calibrationStartSample[i];
      calibrationPhase[i] = CAL_SETTLE_AFTER_RPWM;
      calibrationPhaseStartMs[i] = now;
    }
    else if (phase == CAL_SETTLE_AFTER_RPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DELAY_MS)
    {
      int sample = readModeFilteredSensor(i);
      calibrationStartSample[i] = sample;
      calibrationLastSample[i] = sample;
      calibrationLastSampleMs[i] = now;
      calibrationPhase[i] = CAL_DRIVE_LPWM;
      calibrationPhaseStartMs[i] = now;
      driveActuator(i, false, CALIBRATION_PWM);
    }
    else if (phase == CAL_DRIVE_LPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DURATION_MS)
    {
      stopMotor(i);
      calibrationLastSample[i] = readModeFilteredSensor(i);
      calibrationLpwmTrend[i] = calibrationLastSample[i] - calibrationStartSample[i];
      calibrationPhase[i] = CAL_SETTLE_AFTER_LPWM;
      calibrationPhaseStartMs[i] = now;
    }
    else if (phase == CAL_SETTLE_AFTER_LPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DELAY_MS)
    {
      finishActuatorCalibration(i);
    }
  }

  if (!allDone)
  {
    allDone = true;
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      if (calibrationPhase[i] != CAL_DONE)
      {
        allDone = false;
        break;
      }
    }
  }

  if (allDone)
  {
    calibrationComplete = true;
    debugLog("concurrent direction calibration complete");
  }
}

// ============================================================================
// 8. 阶段 2：移动到初始 50mm 待命 / Stage 2: move to 50 mm and hold
// ============================================================================
// 这个阶段在校准结束后执行：所有执行器都被驱动到 50mm 位置并保持待命。
// After calibration, all actuators are driven to the 50 mm home position and held.
void moveAllActuatorsToInitialPosition()
{
  debugLog("starting concurrent move to 50mm");
  updateCurrentMmFromSensors();
  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetMm[i] = DEFAULT_TARGET_MM;
    lastProgressRaw[i] = currentRaw[i];
    lastProgressMs[i] = now;
    stallWarningReported[i] = false;
    resetPidState(i);
    actuatorMotionActive[i] = isActuatorConfigured(i);
  }

  targetsActive = true;
}

// ============================================================================
// 9. 阶段 3：收到目标后同时移动 / Stage 3: move to target after command
// ============================================================================
// 这个阶段在收到新的目标后执行：所有执行器同时向各自目标位置移动。
// This stage runs when a new target command is received: all actuators move to their targets together.
void updateActuatorMotion()
{
  if (!targetsActive)
  {
    return;
  }

  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!isActuatorConfigured(i) || !actuatorMotionActive[i])
    {
      continue;
    }

    int error = targetMm[i] - currentMm[i];
    if (abs(error) <= TARGET_DEADBAND_MM)
    {
      stopMotor(i);
      resetPidState(i);
      actuatorMotionActive[i] = false;
      Serial.print("DBG actuator ");
      Serial.print(i);
      Serial.print(" reached target=");
      Serial.println(targetMm[i]);
      continue;
    }

    if (abs(currentRaw[i] - lastProgressRaw[i]) >= STALL_RAW_DELTA_THRESHOLD)
    {
      lastProgressRaw[i] = currentRaw[i];
      lastProgressMs[i] = now;
      stallWarningReported[i] = false;
    }

    if (now - lastProgressMs[i] >= STALL_TIMEOUT_MS)
    {
      if (!stallWarningReported[i])
      {
        Serial.print("DBG actuator ");
        Serial.print(i);
        Serial.print(" stall warning current=");
        Serial.print(currentMm[i]);
        Serial.print(" target=");
        Serial.println(targetMm[i]);
        stallWarningReported[i] = true;
      }

      if (STOP_ON_STALL)
      {
        stopMotor(i);
        resetPidState(i);
        actuatorMotionActive[i] = false;
        continue;
      }
    }

    applyActuatorControl(i, error);
  }

  bool anyActive = false;
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (isActuatorConfigured(i) && actuatorMotionActive[i])
    {
      anyActive = true;
      break;
    }
  }

  targetsActive = anyActive;
  if (!targetsActive)
  {
    debugLog("all active actuators stopped");
    sendTelemetry();
  }
}

// ============================================================================
// 10. 串口协议与 JSON 上报 / Serial protocol and JSON reporting
// ============================================================================
void sendError(const char *errorCode)
{
  JSONVar payload;
  payload["error"] = errorCode;
  Serial.println(JSON.stringify(payload));

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG error=");
    Serial.println(errorCode);
  }
}

void sendTelemetry()
{
  JSONVar payload;

  payload["a1_current"] = currentMm[0];
  payload["a1_target"] = targetMm[0];

  payload["a2_current"] = currentMm[1];
  payload["a2_target"] = targetMm[1];

  payload["a3_current"] = currentMm[2];
  payload["a3_target"] = targetMm[2];

  payload["a4_current"] = currentMm[3];
  payload["a4_target"] = targetMm[3];

  Serial.println(JSON.stringify(payload));
}

bool parseCsvTargets(const char *line, int nextTargets[ACTUATOR_COUNT])
{
  if (line[0] != 'T' || line[1] != ',')
  {
    return false;
  }

  const char *cursor = line + 2;

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    char *endPtr;
    long parsedValue = strtol(cursor, &endPtr, 10);

    if (endPtr == cursor)
    {
      return false;
    }

    if (parsedValue < 0 || parsedValue > 800)
    {
      return false;
    }

    nextTargets[i] = (int)parsedValue;
    cursor = endPtr;

    if (i < ACTUATOR_COUNT - 1)
    {
      if (*cursor != ',')
      {
        return false;
      }
      cursor++;
      continue;
    }

    if (*cursor != '\0')
    {
      return false;
    }
  }

  return true;
}

void applyTargets(const int nextTargets[ACTUATOR_COUNT])
{
  unsigned long now = millis();
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetMm[i] = nextTargets[i];
    lastProgressRaw[i] = currentRaw[i];
    lastProgressMs[i] = now;
    stallWarningReported[i] = false;
    resetPidState(i);
    actuatorMotionActive[i] = isActuatorConfigured(i);
  }

  targetsActive = true;
  debugTargets("applied");
}

void processMessageLine(const char *line)
{
  if (DEBUG_SERIAL)
  {
    Serial.print("DBG received line: ");
    Serial.println(line);
  }

  if (line[0] == '\0')
  {
    sendError("empty_message");
    return;
  }

  if (!calibrationComplete)
  {
    sendError("calibration_in_progress");
    return;
  }

  int nextTargets[ACTUATOR_COUNT];
  if (!parseCsvTargets(line, nextTargets))
  {
    sendError("invalid_command");
    return;
  }

  applyTargets(nextTargets);
  sendTelemetry();
}

void handleSerialInput()
{
  while (Serial.available() > 0)
  {
    char ch = (char)Serial.read();

    if (ch == '\r')
    {
      continue;
    }

    if (ch == '\n')
    {
      if (serialLineOverflow)
      {
        sendError("input_overflow");
      }
      else
      {
        serialBuffer[serialBufferIndex] = '\0';
        processMessageLine(serialBuffer);
      }

      serialBufferIndex = 0;
      serialLineOverflow = false;
      continue;
    }

    if (serialLineOverflow)
    {
      continue;
    }

    if (serialBufferIndex < SERIAL_BUFFER_LEN - 1)
    {
      serialBuffer[serialBufferIndex++] = ch;
    }
    else
    {
      serialLineOverflow = true;
    }
  }
}

// ============================================================================
// 11. 初始化与主循环 / Setup and main loop
// ============================================================================
void setup()
{
  Serial.begin(9600);
  debugLog("setup start");

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    pinMode(RPWM_PINS[i], OUTPUT);
    pinMode(LPWM_PINS[i], OUTPUT);
  }

  stopAllMotors();
  startActuatorDirectionCalibration();
  debugLog("setup initialized");
}

void loop()
{
  handleSerialInput();

  unsigned long now = millis();
  if (!calibrationComplete)
  {
    updateActuatorDirectionCalibration();
    if (calibrationComplete && !initialMoveStarted)
    {
      initialMoveStarted = true;
      updateCurrentMmFromSensors();
      sendTelemetry();
      lastTelemetryMs = millis();
      if (calibrationSensorFailure)
      {
        sendError("sensor_not_connected");
      }
      moveAllActuatorsToInitialPosition();
    }
  }

  if (calibrationComplete && now - lastSampleMs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleMs = now;
    updateCurrentMmFromSensors();
    if (targetsActive)
    {
      updateActuatorMotion();
    }
  }

  if (calibrationComplete && now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = now;
    sendTelemetry();
  }
