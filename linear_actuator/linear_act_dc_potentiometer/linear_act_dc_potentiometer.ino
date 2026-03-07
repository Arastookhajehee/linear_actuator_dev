/*
IBT-2 Motor Control Board driven by Arduino Mega.

This sketch controls 4 linear actuators.
Targets are updated by newline-delimited JSON over serial.
*/

#include <Arduino_JSON.h>
#include <stdlib.h>

const int ACTUATOR_COUNT = 4;

const int SENSOR_PINS[ACTUATOR_COUNT] = {A1, A2, A3, A4};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};
const int ACTUATOR_IDS[ACTUATOR_COUNT] = {1, 2, 3, 4};

int targetValues[ACTUATOR_COUNT] = {70,70,70,70};
int lastSensorValues[ACTUATOR_COUNT] = {0, 0, 0, 0};

const int TARGET_DEADBAND = 30;
const int DRIVE_PWM = 70;
const int MEDIAN_SAMPLES = 7;

const unsigned long SAMPLE_INTERVAL_MS = 100;
const unsigned long TELEMETRY_INTERVAL_MS = 1000;
unsigned long lastSampleMs = 0;
unsigned long lastTelemetryMs = 0;

const int SERIAL_BUFFER_LEN = 256;
char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;
bool serialLineOverflow = false;

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

void debugTargets(const char *label, const int values[ACTUATOR_COUNT])
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
    Serial.print(values[i]);
    if (i < ACTUATOR_COUNT - 1)
    {
      Serial.print(",");
    }
  }
  Serial.println("]");
}

void stopMotor(int actuatorIndex)
{
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

void driveTowardTarget(int actuatorIndex, int sensorValue)
{
  int error = targetValues[actuatorIndex] - sensorValue;

  if (abs(error) <= TARGET_DEADBAND)
  {
    stopMotor(actuatorIndex);
    return;
  }

  bool shouldDriveForward = (error > 0);
  if (INVERT_DIRECTION[actuatorIndex])
  {
    shouldDriveForward = !shouldDriveForward;
  }

  if (shouldDriveForward)
  {
    analogWrite(LPWM_PINS[actuatorIndex], 0);
    analogWrite(RPWM_PINS[actuatorIndex], DRIVE_PWM);
  }
  else
  {
    analogWrite(RPWM_PINS[actuatorIndex], 0);
    analogWrite(LPWM_PINS[actuatorIndex], DRIVE_PWM);
  }
}

void sampleSensorsAndDrive()
{
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    lastSensorValues[i] = readMedianSensor(SENSOR_PINS[i]);
    driveTowardTarget(i, lastSensorValues[i]);
  }
}

void sendTelemetry()
{
  JSONVar response;
  JSONVar actuators;

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    JSONVar actuator;
    actuator["id"] = ACTUATOR_IDS[i];
    actuator["current"] = lastSensorValues[i];
    actuator["target"] = targetValues[i];
    actuators[i] = actuator;
  }

  response["actuators"] = actuators;
  Serial.println(JSON.stringify(response));
  debugTargets("telemetry", targetValues);
}

void sendError(const char *errorCode)
{
  JSONVar response;
  response["error"] = errorCode;
  Serial.println(JSON.stringify(response));

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG error=");
    Serial.println(errorCode);
  }
}

bool parseJsonNumber(const JSONVar &value, double &parsedNumber)
{
  String valueType = JSON.typeof(value);
  if (valueType != "number")
  {
    return false;
  }

  String numericText = JSON.stringify(value);
  char *endPtr;
  parsedNumber = strtod(numericText.c_str(), &endPtr);
  if (endPtr == numericText.c_str() || *endPtr != '\0')
  {
    return false;
  }

  return true;
}

bool parseTargetsFromJson(const JSONVar &actuators, int parsedTargets[ACTUATOR_COUNT])
{
  if (JSON.typeof(actuators) != "array")
  {
    debugLog("parseTargetsFromJson: actuators is not an array");
    return false;
  }

  if (JSON.typeof(actuators[ACTUATOR_COUNT]) != "undefined")
  {
    debugLog("parseTargetsFromJson: actuators count is greater than 4");
    return false;
  }

  bool seen[ACTUATOR_COUNT] = {false, false, false, false};

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    JSONVar actuator = actuators[i];
    if (JSON.typeof(actuator) != "object")
    {
      debugLog("parseTargetsFromJson: actuator entry is not an object");
      return false;
    }

    if (!actuator.hasOwnProperty("id") || !actuator.hasOwnProperty("target"))
    {
      debugLog("parseTargetsFromJson: missing id or target");
      return false;
    }

    double parsedId;
    if (!parseJsonNumber(actuator["id"], parsedId))
    {
      debugLog("parseTargetsFromJson: invalid id value");
      return false;
    }

    int actuatorId = (int)parsedId;
    if (actuatorId < 1 || actuatorId > ACTUATOR_COUNT)
    {
      debugLog("parseTargetsFromJson: actuator id out of range");
      return false;
    }

    int targetIndex = actuatorId - 1;
    if (seen[targetIndex])
    {
      debugLog("parseTargetsFromJson: duplicate actuator id");
      return false;
    }
    seen[targetIndex] = true;

    JSONVar targetValue = actuator["target"];
    if (JSON.typeof(targetValue) == "null")
    {
      parsedTargets[targetIndex] = targetValues[targetIndex];
      continue;
    }

    double parsedTarget;
    if (!parseJsonNumber(targetValue, parsedTarget))
    {
      debugLog("parseTargetsFromJson: target is not numeric or null");
      return false;
    }

    if (parsedTarget < 0.0 || parsedTarget > 1023.0)
    {
      debugLog("parseTargetsFromJson: target out of range 0..1023");
      return false;
    }

    parsedTargets[targetIndex] = (int)parsedTarget;
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!seen[i])
    {
      debugLog("parseTargetsFromJson: missing at least one actuator id");
      return false;
    }
  }

  return true;
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

  JSONVar message = JSON.parse(line);
  if (JSON.typeof(message) == "undefined")
  {
    sendError("invalid_json");
    return;
  }

  if (!message.hasOwnProperty("actuators"))
  {
    sendError("missing_actuators");
    return;
  }

  int nextTargets[ACTUATOR_COUNT];
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    nextTargets[i] = targetValues[i];
  }

  JSONVar actuators = message["actuators"];
  if (!parseTargetsFromJson(actuators, nextTargets))
  {
    sendError("invalid_actuators");
    return;
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetValues[i] = nextTargets[i];
  }

  debugTargets("applied", targetValues);

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
  sampleSensorsAndDrive();
  debugTargets("startup", targetValues);
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
    sampleSensorsAndDrive();
  }

  if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = now;
    sendTelemetry();
  }
}

