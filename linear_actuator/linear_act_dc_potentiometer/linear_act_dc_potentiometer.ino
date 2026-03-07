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

int targetValues[ACTUATOR_COUNT] = {100, 100, 100, 100};
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
}

void sendError(const char *errorCode)
{
  JSONVar response;
  response["error"] = errorCode;
  Serial.println(JSON.stringify(response));
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
    return false;
  }

  if (JSON.typeof(actuators[ACTUATOR_COUNT]) != "undefined")
  {
    return false;
  }

  bool seen[ACTUATOR_COUNT] = {false, false, false, false};

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    JSONVar actuator = actuators[i];
    if (JSON.typeof(actuator) != "object")
    {
      return false;
    }

    if (!actuator.hasOwnProperty("id") || !actuator.hasOwnProperty("target"))
    {
      return false;
    }

    double parsedId;
    if (!parseJsonNumber(actuator["id"], parsedId))
    {
      return false;
    }

    int actuatorId = (int)parsedId;
    if (actuatorId < 1 || actuatorId > ACTUATOR_COUNT)
    {
      return false;
    }

    int targetIndex = actuatorId - 1;
    if (seen[targetIndex])
    {
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
      return false;
    }

    if (parsedTarget < 0.0 || parsedTarget > 1023.0)
    {
      return false;
    }

    parsedTargets[targetIndex] = (int)parsedTarget;
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!seen[i])
    {
      return false;
    }
  }

  return true;
}

void processMessageLine(const char *line)
{
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

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    pinMode(RPWM_PINS[i], OUTPUT);
    pinMode(LPWM_PINS[i], OUTPUT);
  }

  stopAllMotors();
  sampleSensorsAndDrive();
  sendTelemetry();
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

