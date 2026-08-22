/*
  Linear actuator controller for Arduino Mega.

  Incoming command protocol is CSV:
    T,<a1_target>,<a2_target>,<a3_target>,<a4_target>

  Outgoing telemetry protocol is flat JSON:
  {
    "a1_current": <number>, "a1_target": <number>,
    "a2_current": <number>, "a2_target": <number>,
    "a3_current": <number>, "a3_target": <number>,
    "a4_current": <number>, "a4_target": <number>
  }
*/

#include <Arduino_JSON.h>
#include <stdlib.h>

const int ACTUATOR_COUNT = 4;

const int SENSOR_PINS[ACTUATOR_COUNT] = {A1, A2, A3, A4};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};
const bool ACTUATOR_ENABLED[ACTUATOR_COUNT] = {true, true, true, true};

const int DEFAULT_TARGET = 100;
const int TARGET_DEADBAND = 1;
const int DRIVE_PWM = 200;
const int MEDIAN_SAMPLES = 7;
const int CALIBRATION_PWM = 255;
const unsigned long CALIBRATION_DURATION_MS = 3000;
const unsigned long CALIBRATION_DELAY_MS = 200;
const int CALIBRATION_SENSOR_DELTA_THRESHOLD = 1;
const int CALIBRATION_PASSES = 2;

const unsigned long SAMPLE_INTERVAL_MS = 100;
const unsigned long TELEMETRY_INTERVAL_MS = 1000;

const int SERIAL_BUFFER_LEN = 256;
char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;
bool serialLineOverflow = false;

int targetValues[ACTUATOR_COUNT] = {
  DEFAULT_TARGET,
  DEFAULT_TARGET,
  DEFAULT_TARGET,
  DEFAULT_TARGET,
};

int currentValues[ACTUATOR_COUNT] = {0, 0, 0, 0};
bool sensorIncreasesWithRPWM[ACTUATOR_COUNT] = {false, false, false, false};

unsigned long lastSampleMs = 0;
unsigned long lastTelemetryMs = 0;

const bool DEBUG_SERIAL = true;

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
    Serial.print(targetValues[i]);
    if (i < ACTUATOR_COUNT - 1)
    {
      Serial.print(",");
    }
  }

  Serial.println("]");
}

bool isActuatorConfigured(int actuatorIndex)
{
  return ACTUATOR_ENABLED[actuatorIndex];
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

int readMedianSensor(int sensorPin)
{
  int samples[MEDIAN_SAMPLES];

  for (int i = 0; i < MEDIAN_SAMPLES; i++)
  {
    samples[i] = analogRead(sensorPin);
  }

  for (int i = 1; i < MEDIAN_SAMPLES; i++)
  {
    int key = samples[i];
    int j = i - 1;

    while (j >= 0 && samples[j] > key)
    {
      samples[j + 1] = samples[j];
      j--;
    }

    samples[j + 1] = key;
  }

  return samples[MEDIAN_SAMPLES / 2];
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

bool calibrateActuatorDirection(int actuatorIndex)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    if (DEBUG_SERIAL)
    {
      Serial.print("DBG actuator ");
      Serial.print(actuatorIndex);
      Serial.println(" skipped: not configured");
    }
    return false;
  }

  int cumulativeRPWMTrend = 0;
  int cumulativeLPWMTrend = 0;

  for (int pass = 0; pass < CALIBRATION_PASSES; pass++)
  {
    int lastSample = readMedianSensor(SENSOR_PINS[actuatorIndex]);
    int minObserved = lastSample;
    int maxObserved = lastSample;

    unsigned long startMs = millis();
    driveActuator(actuatorIndex, true, CALIBRATION_PWM);
    while (millis() - startMs < CALIBRATION_DURATION_MS)
    {
      int sample = readMedianSensor(SENSOR_PINS[actuatorIndex]);
      if (sample < minObserved) minObserved = sample;
      if (sample > maxObserved) maxObserved = sample;
      delay(20);
    }
    stopMotor(actuatorIndex);
    delay(CALIBRATION_DELAY_MS);
    cumulativeRPWMTrend += (maxObserved - minObserved);

    lastSample = readMedianSensor(SENSOR_PINS[actuatorIndex]);
    minObserved = lastSample;
    maxObserved = lastSample;

    startMs = millis();
    driveActuator(actuatorIndex, false, CALIBRATION_PWM);
    while (millis() - startMs < CALIBRATION_DURATION_MS)
    {
      int sample = readMedianSensor(SENSOR_PINS[actuatorIndex]);
      if (sample < minObserved) minObserved = sample;
      if (sample > maxObserved) maxObserved = sample;
      delay(20);
    }
    stopMotor(actuatorIndex);
    delay(CALIBRATION_DELAY_MS);
    cumulativeLPWMTrend += (maxObserved - minObserved);
  }

  int rpwmTrend = cumulativeRPWMTrend / CALIBRATION_PASSES;
  int lpwmTrend = cumulativeLPWMTrend / CALIBRATION_PASSES;

  bool increasesWithRPWM = false;
  if (abs(rpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    increasesWithRPWM = (rpwmTrend > 0);
  }
  else if (abs(lpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    increasesWithRPWM = (lpwmTrend < 0);
  }
  else
  {
    debugLog("calibration no clear change");
  }

  sensorIncreasesWithRPWM[actuatorIndex] = increasesWithRPWM;

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG cal actuator ");
    Serial.print(actuatorIndex);
    Serial.print(" rpwm_trend=");
    Serial.print(rpwmTrend);
    Serial.print(" lpwm_trend=");
    Serial.print(lpwmTrend);
    Serial.print(" increases_with_rpwm=");
    Serial.println(increasesWithRPWM ? 1 : 0);
  }

  return true;
}

void calibrateActuatorDirections()
{
  debugLog("starting direction calibration");
  stopAllMotors();

  for (int pass = 0; pass < CALIBRATION_PASSES; pass++)
  {
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      driveActuator(i, true, CALIBRATION_PWM);
    }
    delay(CALIBRATION_DURATION_MS);
    stopAllMotors();
    delay(CALIBRATION_DELAY_MS);

    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      driveActuator(i, false, CALIBRATION_PWM);
    }
    delay(CALIBRATION_DURATION_MS);
    stopAllMotors();
    delay(CALIBRATION_DELAY_MS);
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    calibrateActuatorDirection(i);
  }

  debugLog("direction calibration complete");
}

void driveTowardTarget(int actuatorIndex, int sensorValue)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    return;
  }

  int error = targetValues[actuatorIndex] - sensorValue;
  if (abs(error) <= TARGET_DEADBAND)
  {
    stopMotor(actuatorIndex);
    return;
  }

  bool shouldIncreaseSensor = (error > 0);
  bool useRPWM = sensorIncreasesWithRPWM[actuatorIndex];

  if (INVERT_DIRECTION[actuatorIndex])
  {
    useRPWM = !useRPWM;
  }

  if (!shouldIncreaseSensor)
  {
    useRPWM = !useRPWM;
  }

  driveActuator(actuatorIndex, useRPWM, DRIVE_PWM);
}

void sampleAndDriveAllActuators()
{
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    currentValues[i] = readMedianSensor(SENSOR_PINS[i]);
    driveTowardTarget(i, currentValues[i]);
  }
}

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

  payload["a1_current"] = currentValues[0];
  payload["a1_target"] = targetValues[0];

  payload["a2_current"] = currentValues[1];
  payload["a2_target"] = targetValues[1];

  payload["a3_current"] = currentValues[2];
  payload["a3_target"] = targetValues[2];

  payload["a4_current"] = currentValues[3];
  payload["a4_target"] = targetValues[3];

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

    if (parsedValue < 0 || parsedValue > 1023)
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
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetValues[i] = nextTargets[i];
  }
}

/*
  Command receive and parse section:
  - This block receives control commands from the host through the serial port.
  - Characters are buffered until a newline arrives, then the complete line is parsed.
  - The expected command format is CSV: T,<a1_target>,<a2_target>,<a3_target>,<a4_target>
  - If the format is valid, the targets are applied; otherwise an error response is sent.

  指令接收与解析部分：
  - 这里负责从串口接收上位机发送的控制指令。
  - 字符会被缓存，直到遇到换行符后再组成完整的一行进行解析。
  - 预期的指令格式为 CSV：T,<a1_target>,<a2_target>,<a3_target>,<a4_target>
  - 如果格式合法，则更新目标值；否则返回错误响应。
*/
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

  int nextTargets[ACTUATOR_COUNT];
  if (!parseCsvTargets(line, nextTargets))
  {
    sendError("invalid_command");
    return;
  }

  applyTargets(nextTargets);
  debugTargets("applied");
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
  calibrateActuatorDirections();
  sampleAndDriveAllActuators();
  debugTargets("startup");
  sendTelemetry();
  debugLog("setup complete");
}

void loop()
{
  handleSerialInput();
  unsigned long now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleMs = now;
    sampleAndDriveAllActuators();
  }

  if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = now;
    sendTelemetry();
  }
}
