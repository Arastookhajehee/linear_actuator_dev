/*
  800 mm 线性执行器：自动标定 + 串口位置测试
  --------------------------------------------------------------
  适用硬件（与当前程序一致）：
    位置传感器：A0
    IBT-2 RPWM：D5
    IBT-2 LPWM：D6
    Arduino Uno / Nano ADC：0..1023

  重要安全说明：
  1) 本程序不在上电后自动运动。打开串口监视器后，发送 a 才开始标定。
  2) 标定时会向两个机械端点运动；必须确认两端微动限位开关有效、驱动接线正确，
     且运动区域内没有人手或障碍物。
  3) 本程序依据“电机持续驱动时，传感器信号在一段时间内不再变化”判断到达端点。
     因此它无法区分“限位开关已断电”和“机构卡滞”。超时只是一层额外保护。
  4) 任意时刻在串口发送 x，可立即停止电机并退出当前动作。

  串口监视器：115200 baud，行结束符选择 Newline 或 Both NL & CR。

  命令：
    a          开始自动标定（方向探测 -> 0 mm 端 -> 800 mm 端 -> 保存 EEPROM）
    0..800     标定成功后，移动到指定毫米位置，例如 320
    p          打印当前标定与实时状态
    x          立即停止当前标定/运动
    e          清除 EEPROM 中保存的标定数据
    h          帮助

  坐标约定：
    默认 ZERO_IS_LOW_SENSOR_SIGNAL = true。
    即 ADC 原始值较低的机械端定义为 0 mm，ADC 原始值较高的端定义为 800 mm。
    若实际机械“回缩端”反而对应较高 ADC 值，将该常量改为 false 后重新标定。
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 1. 硬件与坐标配置
// ============================================================================
const uint8_t SENSOR_PIN = A0;
const uint8_t RPWM_PIN   = 5;
const uint8_t LPWM_PIN   = 6;

const long SERIAL_BAUD = 115200;
const int ADC_MAX = 1023;           // Arduino Uno / Nano / Mega 为 1023
const int STROKE_MM = 800;          // 标准机械行程
const int PWM_MAX = 255;

// true：较低 ADC 信号端 = 0 mm；较高 ADC 信号端 = 800 mm。
// false：较高 ADC 信号端 = 0 mm；较低 ADC 信号端 = 800 mm。
const bool ZERO_IS_LOW_SENSOR_SIGNAL = true;

// ============================================================================
// 2. 可调标定参数
// ============================================================================
// 方向探测：每个方向短暂运动一次，观察原始 ADC 的正/负变化。
const int PROBE_PWM = 150;
const unsigned long PROBE_MS = 800;
const int MIN_PROBE_DELTA_RAW = 3;

// 去端点时的速度：按你的要求，800 mm 满行程端以 255 PWM 运行。
const int HOME_PWM = 180;
const int FULL_STROKE_PWM = 255;

// 端点判定：驱动中，若一个 1200 ms 窗口内 ADC 摆动范围 <= 2，且该状态维持 1500 ms，
// 则判定限位开关已经断电或机构已停止。
const unsigned long END_SAMPLE_MS = 50;
const unsigned long END_WINDOW_MS = 1200;
const int END_WINDOW_RANGE_RAW_MAX = 2;
const unsigned long END_STABLE_CONFIRM_MS = 1500;
const unsigned long END_SETTLE_MS = 300;

// 单程最大运行时间。800 mm 低速执行器可能很慢，设置为 180 秒。
const unsigned long ENDPOINT_TIMEOUT_MS = 180000UL;

// 两端 ADC 跨度过小，说明传感器、接线或端点判断异常。
const int MIN_VALID_SPAN_RAW = 50;

// ============================================================================
// 3. 串口位置测试参数（标定后使用）
// ============================================================================
const unsigned long CONTROL_DT_MS = 20;
const int TARGET_TOLERANCE_MM = 2;
const unsigned long MOVE_TIMEOUT_MS = 180000UL;
const unsigned long MOVE_STALL_TIMEOUT_MS = 3000;
const int MOVE_STALL_MIN_PROGRESS_MM = 2;

// ============================================================================
// 4. EEPROM 标定数据
// ============================================================================
const uint32_t CAL_MAGIC = 0x4C413830UL;   // "LA80"
const uint16_t CAL_VERSION = 1;

struct __attribute__((packed)) CalibrationData {
  uint32_t magic;
  uint16_t version;
  int16_t rawAt0;
  int16_t rawAt800;
  uint8_t rpwmIsExtend;
  uint8_t zeroIsLowSensor;
  uint16_t checksum;
};

CalibrationData cal;
bool calValid = false;

uint16_t calcChecksum(const CalibrationData &data)
{
  const uint8_t *p = (const uint8_t *)&data;
  uint16_t c = 0xA55A;

  for (uint8_t i = 0; i < sizeof(CalibrationData) - sizeof(data.checksum); i++) {
    c = (uint16_t)((c << 5) | (c >> 11));
    c ^= p[i];
  }

  return c;
}

bool calibrationFieldsValid(const CalibrationData &data)
{
  if (data.magic != CAL_MAGIC || data.version != CAL_VERSION) return false;
  if (data.checksum != calcChecksum(data)) return false;
  if (data.zeroIsLowSensor != (ZERO_IS_LOW_SENSOR_SIGNAL ? 1 : 0)) return false;
  if (data.rawAt0 < 0 || data.rawAt0 > ADC_MAX) return false;
  if (data.rawAt800 < 0 || data.rawAt800 > ADC_MAX) return false;
  if (abs((int)data.rawAt800 - (int)data.rawAt0) < MIN_VALID_SPAN_RAW) return false;
  return true;
}

void saveCalibration()
{
  cal.magic = CAL_MAGIC;
  cal.version = CAL_VERSION;
  cal.zeroIsLowSensor = ZERO_IS_LOW_SENSOR_SIGNAL ? 1 : 0;
  cal.checksum = calcChecksum(cal);
  EEPROM.put(0, cal);
  calValid = calibrationFieldsValid(cal);
}

void loadCalibration()
{
  EEPROM.get(0, cal);
  calValid = calibrationFieldsValid(cal);
}

void eraseCalibration()
{
  CalibrationData empty;
  memset(&empty, 0, sizeof(empty));
  EEPROM.put(0, empty);
  calValid = false;
  Serial.println(F("EEPROM_CALIBRATION_CLEARED"));
}

// ============================================================================
// 5. 电机与传感器基础函数
// ============================================================================
bool rpwmIsExtend = true;  // 标定/EEPROM 后得到：RPWM 是否对应 0 -> 800 的方向。

void stopMotor()
{
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
}

void driveRawRPWM(int pwm)
{
  pwm = constrain(pwm, 0, PWM_MAX);
  analogWrite(LPWM_PIN, 0);
  analogWrite(RPWM_PIN, pwm);
}

void driveRawLPWM(int pwm)
{
  pwm = constrain(pwm, 0, PWM_MAX);
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, pwm);
}

void driveExtendPwm(int pwm)
{
  if (pwm <= 0) {
    stopMotor();
    return;
  }

  if (rpwmIsExtend) driveRawRPWM(pwm);
  else driveRawLPWM(pwm);
}

void driveRetractPwm(int pwm)
{
  if (pwm <= 0) {
    stopMotor();
    return;
  }

  if (rpwmIsExtend) driveRawLPWM(pwm);
  else driveRawRPWM(pwm);
}

const uint8_t MAX_MEDIAN_SAMPLES = 31;

int readRawMedian(uint8_t count)
{
  int values[MAX_MEDIAN_SAMPLES];

  if (count < 1) count = 1;
  if (count > MAX_MEDIAN_SAMPLES) count = MAX_MEDIAN_SAMPLES;
  if ((count & 1) == 0) count--;   // 强制奇数，方便取中位数。

  for (uint8_t i = 0; i < count; i++) {
    values[i] = analogRead(SENSOR_PIN);
    delayMicroseconds(350);
  }

  for (uint8_t i = 1; i < count; i++) {
    int key = values[i];
    int j = i - 1;
    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      j--;
    }
    values[j + 1] = key;
  }

  return values[count / 2];
}

int rawToPositionMm(int raw)
{
  if (!calValid) return -1;

  const long span = (long)cal.rawAt800 - (long)cal.rawAt0;
  if (span == 0) return -1;

  long pos = ((long)(raw - cal.rawAt0) * (long)STROKE_MM) / span;
  if (pos < 0) pos = 0;
  if (pos > STROKE_MM) pos = STROKE_MM;
  return (int)pos;
}

int readPositionMm()
{
  return rawToPositionMm(readRawMedian(11));
}

// ============================================================================
// 6. 系统状态与串口输入
// ============================================================================
enum SystemMode {
  MODE_IDLE,
  MODE_CALIBRATING,
  MODE_MOVING,
  MODE_ERROR
};

SystemMode mode = MODE_IDLE;
bool abortRequested = false;

int targetMm = 0;
int currentMm = -1;
unsigned long moveStartMs = 0;
unsigned long moveLastControlMs = 0;
unsigned long moveLastProgressMs = 0;
int moveProgressReferenceMm = -1;

const uint8_t SERIAL_BUFFER_LEN = 24;
char serialBuffer[SERIAL_BUFFER_LEN];
uint8_t serialIndex = 0;
bool serialOverflow = false;

void printHelp();
void printStatus();
void startAutomaticCalibration();
void startMoveTo(int newTargetMm);

void stopEverythingToIdle(const __FlashStringHelper *reason)
{
  stopMotor();
  mode = MODE_IDLE;
  abortRequested = true;
  Serial.print(F("STOPPED "));
  Serial.println(reason);
}

bool isAllDigits(const char *text)
{
  if (*text == '\0') return false;
  while (*text) {
    if (*text < '0' || *text > '9') return false;
    text++;
  }
  return true;
}

bool lineEqualsIgnoreCase(const char *line, const char *cmd)
{
  while (*line && *cmd) {
    char c1 = *line;
    char c2 = *cmd;
    if (c1 >= 'a' && c1 <= 'z') c1 -= 'a' - 'A';
    if (c2 >= 'a' && c2 <= 'z') c2 -= 'a' - 'A';
    if (c1 != c2) return false;
    line++;
    cmd++;
  }
  return *line == '\0' && *cmd == '\0';
}

bool lineStartsWithIgnoreCase(const char *line, const char *prefix)
{
  while (*prefix) {
    if (*line == '\0') return false;
    char c1 = *line;
    char c2 = *prefix;
    if (c1 >= 'a' && c1 <= 'z') c1 -= 'a' - 'A';
    if (c2 >= 'a' && c2 <= 'z') c2 -= 'a' - 'A';
    if (c1 != c2) return false;
    line++;
    prefix++;
  }
  return true;
}

void printStatusJson()
{
  const int raw = readRawMedian(11);
  const int pos = rawToPositionMm(raw);

  Serial.print(F("{"));
  Serial.print(F("\"mode\":\""));
  Serial.print(modeName());
  Serial.print(F("\",\"raw\":"));
  Serial.print(raw);
  Serial.print(F(",\"pos_mm\":"));
  Serial.print(pos);
  Serial.print(F(",\"target_mm\":"));
  Serial.print(targetMm);
  Serial.print(F(",\"calValid\":"));
  Serial.print(calValid ? 1 : 0);
  Serial.println(F("}"));
}

void processSerialLine(char *line)
{
  // 去掉首尾空格。
  while (*line == ' ' || *line == '\t') line++;
  char *end = line + strlen(line);
  while (end > line && (end[-1] == ' ' || end[-1] == '\t')) {
    *--end = '\0';
  }

  if (line[0] == '\0') return;

  if (lineEqualsIgnoreCase(line, "x") || lineEqualsIgnoreCase(line, "stop")) {
    stopEverythingToIdle(F("SERIAL_STOP"));
    return;
  }

  if (lineEqualsIgnoreCase(line, "h") || lineEqualsIgnoreCase(line, "help") || lineEqualsIgnoreCase(line, "?")) {
    printHelp();
    return;
  }

  if (lineEqualsIgnoreCase(line, "p") || lineEqualsIgnoreCase(line, "status")) {
    printStatus();
    printStatusJson();
    return;
  }

  if (lineEqualsIgnoreCase(line, "e") || lineEqualsIgnoreCase(line, "erase")) {
    if (mode == MODE_CALIBRATING || mode == MODE_MOVING) {
      Serial.println(F("BUSY_USE_X_FIRST"));
    } else {
      eraseCalibration();
    }
    return;
  }

  if (lineEqualsIgnoreCase(line, "a") || lineEqualsIgnoreCase(line, "cal") || lineEqualsIgnoreCase(line, "calibrate")) {
    if (mode == MODE_CALIBRATING || mode == MODE_MOVING) {
      Serial.println(F("BUSY_USE_X_FIRST"));
    } else {
      startAutomaticCalibration();
    }
    return;
  }

  if (lineStartsWithIgnoreCase(line, "t,") || lineStartsWithIgnoreCase(line, "m,")) {
    const char *valueText = line + 2;
    if (!isAllDigits(valueText)) {
      Serial.println(F("INVALID_POSITION_USE_0_TO_800"));
      return;
    }
    long requested = strtol(valueText, NULL, 10);
    if (requested < 0 || requested > STROKE_MM) {
      Serial.println(F("INVALID_POSITION_USE_0_TO_800"));
      return;
    }

    if (!calValid) {
      Serial.println(F("NO_VALID_CALIBRATION_SEND_A_FIRST"));
      return;
    }

    if (mode == MODE_CALIBRATING) {
      Serial.println(F("BUSY_CALIBRATING_USE_X_TO_ABORT"));
      return;
    }

    startMoveTo((int)requested);
    return;
  }

  if (isAllDigits(line)) {
    long requested = strtol(line, NULL, 10);
    if (requested < 0 || requested > STROKE_MM) {
      Serial.println(F("INVALID_POSITION_USE_0_TO_800"));
      return;
    }

    if (!calValid) {
      Serial.println(F("NO_VALID_CALIBRATION_SEND_A_FIRST"));
      return;
    }

    if (mode == MODE_CALIBRATING) {
      Serial.println(F("BUSY_CALIBRATING_USE_X_TO_ABORT"));
      return;
    }

    startMoveTo((int)requested);
    return;
  }

  Serial.println(F("UNKNOWN_COMMAND_SEND_H"));
}

void handleSerial()
{
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();

    if (ch == '\r' || ch == '\n') {
      if (serialOverflow) {
        Serial.println(F("SERIAL_LINE_TOO_LONG"));
      } else {
        serialBuffer[serialIndex] = '\0';
        processSerialLine(serialBuffer);
      }
      serialIndex = 0;
      serialOverflow = false;
      continue;
    }

    if (serialOverflow) continue;

    if (serialIndex < SERIAL_BUFFER_LEN - 1) {
      serialBuffer[serialIndex++] = ch;
    } else {
      serialOverflow = true;
    }
  }
}

bool waitWithSerial(unsigned long waitMs)
{
  unsigned long beginMs = millis();
  while (millis() - beginMs < waitMs) {
    handleSerial();
    if (abortRequested) {
      stopMotor();
      return false;
    }
    delay(5);
  }
  return true;
}

// ============================================================================
// 7. 自动方向判断
// ============================================================================
int runRawDirectionProbe(bool useRPWM)
{
  const int before = readRawMedian(11);
  const unsigned long beginMs = millis();

  while (millis() - beginMs < PROBE_MS) {
    if (useRPWM) driveRawRPWM(PROBE_PWM);
    else driveRawLPWM(PROBE_PWM);

    handleSerial();
    if (abortRequested) {
      stopMotor();
      return 0;
    }
    delay(5);
  }

  stopMotor();
  if (!waitWithSerial(180)) return 0;

  const int after = readRawMedian(11);
  return after - before;
}

bool detectMotorDirection()
{
  Serial.println(F("CAL_STEP1_DIRECTION_PROBE_START"));
  Serial.println(F("CAL_PROBE_RPWM"));
  int dR = runRawDirectionProbe(true);
  if (abortRequested) return false;

  Serial.println(F("CAL_PROBE_LPWM"));
  int dL = runRawDirectionProbe(false);
  if (abortRequested) return false;

  Serial.print(F("CAL_DIRECTION_DELTAS RPWM="));
  Serial.print(dR);
  Serial.print(F(" LPWM="));
  Serial.println(dL);

  bool rpwmIncreasesSensor = true;

  if (abs(dR) >= MIN_PROBE_DELTA_RAW) {
    rpwmIncreasesSensor = (dR > 0);
  } else if (abs(dL) >= MIN_PROBE_DELTA_RAW) {
    // LPWM 增加信号，则 RPWM 必然是降低信号；反之亦然。
    rpwmIncreasesSensor = (dL < 0);
  } else {
    Serial.println(F("CAL_ERROR_DIRECTION_NO_SENSOR_CHANGE"));
    return false;
  }

  // “伸出”定义为从 0 mm 指向 800 mm 的正坐标方向。
  if (ZERO_IS_LOW_SENSOR_SIGNAL) {
    rpwmIsExtend = rpwmIncreasesSensor;
  } else {
    rpwmIsExtend = !rpwmIncreasesSensor;
  }

  Serial.print(F("CAL_RPWM_INCREASES_SENSOR="));
  Serial.println(rpwmIncreasesSensor ? 1 : 0);
  Serial.print(F("CAL_RPWM_IS_EXTEND="));
  Serial.println(rpwmIsExtend ? 1 : 0);
  Serial.println(F("CAL_STEP1_DIRECTION_PROBE_DONE"));
  return true;
}

// ============================================================================
// 8. 去机械端点：传感器稳定判据
// ============================================================================
enum TravelDirection { GO_RETRACT, GO_EXTEND };

bool driveUntilStableEnd(TravelDirection direction, int pwm, const __FlashStringHelper *label, int &capturedRaw)
{
  Serial.print(F("CAL_STEP_"));
  Serial.print(label);
  Serial.println(F("_START"));

  const unsigned long startMs = millis();
  unsigned long lastSampleMs = 0;
  unsigned long windowStartMs = millis();
  unsigned long stableStartMs = 0;
  unsigned long lastReportMs = 0;

  int seed = readRawMedian(9);
  int windowMin = seed;
  int windowMax = seed;
  int lastRaw = seed;

  while (millis() - startMs < ENDPOINT_TIMEOUT_MS) {
    if (direction == GO_RETRACT) driveRetractPwm(pwm);
    else driveExtendPwm(pwm);

    handleSerial();
    if (abortRequested) {
      stopMotor();
      return false;
    }

    const unsigned long now = millis();

    if (now - lastSampleMs >= END_SAMPLE_MS) {
      lastSampleMs = now;
      lastRaw = readRawMedian(7);

      if (lastRaw < windowMin) windowMin = lastRaw;
      if (lastRaw > windowMax) windowMax = lastRaw;
    }

    if (now - lastReportMs >= 500) {
      lastReportMs = now;
      Serial.print(F("CAL_PROGRESS "));
      Serial.print(label);
      Serial.print(F(" RAW="));
      Serial.print(lastRaw);
      Serial.print(F(" RANGE="));
      Serial.println(windowMax - windowMin);
    }

    if (now - windowStartMs >= END_WINDOW_MS) {
      const int range = windowMax - windowMin;

      if (range <= END_WINDOW_RANGE_RAW_MAX) {
        if (stableStartMs == 0) {
          stableStartMs = now;
          Serial.print(F("CAL_SIGNAL_STABLE_CANDIDATE "));
          Serial.println(label);
        }

        if (now - stableStartMs >= END_STABLE_CONFIRM_MS) {
          stopMotor();
          Serial.print(F("CAL_END_CONFIRMED "));
          Serial.println(label);

          if (!waitWithSerial(END_SETTLE_MS)) return false;
          capturedRaw = readRawMedian(31);
          Serial.print(F("CAL_CAPTURED_RAW_"));
          Serial.print(label);
          Serial.print(F("="));
          Serial.println(capturedRaw);
          return true;
        }
      } else {
        stableStartMs = 0;
      }

      // 新窗口从当前读数开始。
      windowStartMs = now;
      windowMin = lastRaw;
      windowMax = lastRaw;
    }

    delay(5);
  }

  stopMotor();
  Serial.print(F("CAL_ERROR_TIMEOUT_"));
  Serial.println(label);
  return false;
}

// ============================================================================
// 9. 自动标定主流程
// ============================================================================
void printCalibrationResult()
{
  if (!calValid) {
    Serial.println(F("CALIBRATION_NOT_VALID"));
    return;
  }

  const long span = (long)cal.rawAt800 - (long)cal.rawAt0;

  Serial.println(F("----------------------------------------"));
  Serial.println(F("CALIBRATION_OK"));
  Serial.print(F("CAL_RAW_AT_0="));
  Serial.println(cal.rawAt0);
  Serial.print(F("CAL_RAW_AT_800="));
  Serial.println(cal.rawAt800);
  Serial.print(F("CAL_RAW_SPAN="));
  Serial.println(span);
  Serial.print(F("CAL_RPWM_IS_EXTEND="));
  Serial.println(cal.rpwmIsExtend ? 1 : 0);
  Serial.print(F("CAL_ZERO_IS_LOW_SENSOR="));
  Serial.println(cal.zeroIsLowSensor ? 1 : 0);
  Serial.println(F("CAL_EEPROM_SAVED=1"));
  Serial.println(F("FORMULA: pos_mm = clamp((raw - RAW_AT_0) * 800 / (RAW_AT_800 - RAW_AT_0), 0, 800)"));
  Serial.println(F("----------------------------------------"));
}

void calibrationFailed(const __FlashStringHelper *reason)
{
  stopMotor();

  if (abortRequested) {
    mode = MODE_IDLE;
    Serial.println(F("CAL_ABORTED"));
  } else {
    mode = MODE_ERROR;
    Serial.print(F("CAL_FAILED "));
    Serial.println(reason);
  }
}

void startAutomaticCalibration()
{
  abortRequested = false;
  mode = MODE_CALIBRATING;
  stopMotor();

  Serial.println(F("========================================"));
  Serial.println(F("CAL_START"));
  Serial.println(F("CAL_WARNING_VERIFY_LIMIT_SWITCHES_AND_CLEAR_TRAVEL_PATH"));

  if (!detectMotorDirection()) {
    calibrationFailed(F("DIRECTION_PROBE"));
    return;
  }

  int rawAt0 = 0;
  int rawAt800 = 0;

  // 先去 0 mm 端。程序会持续驱动，直到 ADC 在规定时间内不再变化。
  if (!driveUntilStableEnd(GO_RETRACT, HOME_PWM, F("HOME_0MM"), rawAt0)) {
    calibrationFailed(F("HOME_0MM"));
    return;
  }

  // 再去 800 mm 端。依照需求，这一段使用 PWM=255。
  if (!driveUntilStableEnd(GO_EXTEND, FULL_STROKE_PWM, F("FULL_800MM"), rawAt800)) {
    calibrationFailed(F("FULL_800MM"));
    return;
  }

  const int span = rawAt800 - rawAt0;
  if (abs(span) < MIN_VALID_SPAN_RAW) {
    Serial.print(F("CAL_ERROR_SPAN_TOO_SMALL span="));
    Serial.println(span);
    calibrationFailed(F("SENSOR_SPAN"));
    return;
  }

  memset(&cal, 0, sizeof(cal));
  cal.rawAt0 = rawAt0;
  cal.rawAt800 = rawAt800;
  cal.rpwmIsExtend = rpwmIsExtend ? 1 : 0;
  cal.zeroIsLowSensor = ZERO_IS_LOW_SENSOR_SIGNAL ? 1 : 0;
  saveCalibration();

  if (!calValid) {
    calibrationFailed(F("EEPROM_WRITE"));
    return;
  }

  rpwmIsExtend = (cal.rpwmIsExtend != 0);
  currentMm = readPositionMm();
  targetMm = currentMm;
  mode = MODE_IDLE;

  printCalibrationResult();
  Serial.print(F("CAL_DONE_CURRENT_POS_MM="));
  Serial.println(currentMm);
  Serial.println(F("READY_FOR_TEST_SEND_A_NUMBER_0_TO_800"));
}

// ============================================================================
// 10. 标定后的闭环位置测试
// ============================================================================
int chooseMovePwm(int absErrorMm)
{
  if (absErrorMm > 160) return 255;
  if (absErrorMm > 70)  return 190;
  if (absErrorMm > 25)  return 135;
  if (absErrorMm > 8)   return 90;
  return 60;
}

void startMoveTo(int newTargetMm)
{
  if (!calValid) {
    Serial.println(F("NO_VALID_CALIBRATION_SEND_A_FIRST"));
    return;
  }

  abortRequested = false;
  targetMm = constrain(newTargetMm, 0, STROKE_MM);
  currentMm = readPositionMm();

  if (currentMm < 0) {
    stopMotor();
    mode = MODE_ERROR;
    Serial.println(F("MOVE_ERROR_SENSOR_OR_CALIBRATION"));
    return;
  }

  if (abs(targetMm - currentMm) <= TARGET_TOLERANCE_MM) {
    stopMotor();
    mode = MODE_IDLE;
    Serial.print(F("MOVE_ALREADY_AT_TARGET POS_MM="));
    Serial.println(currentMm);
    return;
  }

  moveStartMs = millis();
  moveLastControlMs = 0;
  moveLastProgressMs = millis();
  moveProgressReferenceMm = currentMm;
  mode = MODE_MOVING;

  Serial.print(F("MOVE_START TARGET_MM="));
  Serial.print(targetMm);
  Serial.print(F(" CURRENT_MM="));
  Serial.println(currentMm);
}

void moveFailed(const __FlashStringHelper *reason)
{
  stopMotor();
  mode = MODE_ERROR;
  Serial.print(F("MOVE_FAILED "));
  Serial.println(reason);
}

void updateMove()
{
  const unsigned long now = millis();

  if (now - moveLastControlMs < CONTROL_DT_MS) return;
  moveLastControlMs = now;

  currentMm = readPositionMm();
  if (currentMm < 0) {
    moveFailed(F("POSITION_READ"));
    return;
  }

  const int error = targetMm - currentMm;
  const int absError = abs(error);

  if (absError <= TARGET_TOLERANCE_MM) {
    stopMotor();
    mode = MODE_IDLE;
    Serial.print(F("MOVE_DONE TARGET_MM="));
    Serial.print(targetMm);
    Serial.print(F(" POS_MM="));
    Serial.print(currentMm);
    Serial.print(F(" ERROR_MM="));
    Serial.println(error);
    return;
  }

  if (abs(currentMm - moveProgressReferenceMm) >= MOVE_STALL_MIN_PROGRESS_MM) {
    moveProgressReferenceMm = currentMm;
    moveLastProgressMs = now;
  }

  if (now - moveLastProgressMs > MOVE_STALL_TIMEOUT_MS) {
    moveFailed(F("NO_POSITION_CHANGE"));
    return;
  }

  if (now - moveStartMs > MOVE_TIMEOUT_MS) {
    moveFailed(F("TIMEOUT"));
    return;
  }

  const int pwm = chooseMovePwm(absError);
  if (error > 0) driveExtendPwm(pwm);
  else driveRetractPwm(pwm);
}

// ============================================================================
// 11. 状态输出与主循环
// ============================================================================
const __FlashStringHelper *modeName()
{
  switch (mode) {
    case MODE_IDLE:        return F("IDLE");
    case MODE_CALIBRATING: return F("CALIBRATING");
    case MODE_MOVING:      return F("MOVING");
    case MODE_ERROR:       return F("ERROR");
  }
  return F("UNKNOWN");
}

void printStatus()
{
  const int raw = readRawMedian(11);
  const int pos = rawToPositionMm(raw);

  Serial.println(F("----------------------------------------"));
  Serial.print(F("STATE="));
  Serial.println(modeName());
  Serial.print(F("RAW_NOW="));
  Serial.println(raw);
  Serial.print(F("POS_NOW_MM="));
  Serial.println(pos);
  Serial.print(F("TARGET_MM="));
  Serial.println(targetMm);
  Serial.print(F("EEPROM_CAL_VALID="));
  Serial.println(calValid ? 1 : 0);

  if (calValid) printCalibrationResult();
  Serial.println(F("----------------------------------------"));
}

void printHelp()
{
  Serial.println(F("----------------------------------------"));
  Serial.println(F("800 mm Linear Actuator Auto Calibration"));
  Serial.println(F("Commands:"));
  Serial.println(F("  a | cal | calibrate       : start automatic calibration"));
  Serial.println(F("  t,<0..800> | m,<0..800>    : move to that position in millimeters"));
  Serial.println(F("  0..800                   : move to that position in millimeters"));
  Serial.println(F("  p | status               : print calibration and status"));
  Serial.println(F("  x | stop                 : emergency stop current activity"));
  Serial.println(F("  e | erase                : erase saved EEPROM calibration"));
  Serial.println(F("  h | help | ?             : help"));
  Serial.println(F("Coordinate: 0 mm -> 800 mm; check ZERO_IS_LOW_SENSOR_SIGNAL before calibration."));
  Serial.println(F("Also outputs JSON status on status command."));
  Serial.println(F("----------------------------------------"));
}

void setup()
{
  Serial.begin(SERIAL_BAUD);
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  stopMotor();

  delay(300);
  loadCalibration();

  if (calValid) {
    rpwmIsExtend = (cal.rpwmIsExtend != 0);
    currentMm = readPositionMm();
    targetMm = currentMm;
    Serial.println(F("BOOT_VALID_CALIBRATION_LOADED"));
    printCalibrationResult();
  } else {
    Serial.println(F("BOOT_NO_VALID_CALIBRATION_SEND_A_TO_CALIBRATE"));
  }

  printHelp();
}

void loop()
{
  handleSerial();

  if (mode == MODE_MOVING) {
    updateMove();
  } else if (mode == MODE_IDLE) {
    stopMotor();
  }

  static unsigned long lastTelemetryMs = 0;
  const unsigned long now = millis();

  if (mode == MODE_MOVING && now - lastTelemetryMs >= 250) {
    lastTelemetryMs = now;
    Serial.print(F("MOVE_STATUS TARGET_MM="));
    Serial.print(targetMm);
    Serial.print(F(" POS_MM="));
    Serial.print(currentMm);
    Serial.print(F(" ERROR_MM="));
    Serial.println(targetMm - currentMm);
  }
}
