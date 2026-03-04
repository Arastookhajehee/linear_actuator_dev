/*
IBT-2 Motor Control Board driven by Arduino Mega.

This sketch controls 4 linear actuators.
Targets are updated by newline-delimited JSON over serial.
*/

#include <Arduino_JSON.h>

const int ACTUATOR_COUNT = 4;

const int SENSOR_PINS[ACTUATOR_COUNT] = {A1, A2, A3, A4};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};

int targetValues[ACTUATOR_COUNT] = {250, 260, 270, 280};
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

long lastCommandId = 0;

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

void sendTelemetry(long messageId)
{
  JSONVar response;
  JSONVar sensorValues;
  JSONVar linActs;

  response["id"] = messageId;
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    sensorValues[i] = lastSensorValues[i];
    linActs[i] = targetValues[i];
  }

  response["sensor_values"] = sensorValues;
  response["lin_acts"] = linActs;
  Serial.println(JSON.stringify(response));
}

void sendError(long messageId, const char *errorCode)
{
  JSONVar response;
  response["id"] = messageId;
  response["error"] = errorCode;
  Serial.println(JSON.stringify(response));
}

bool parseTargetsFromJson(const JSONVar &linActs, int parsedTargets[ACTUATOR_COUNT])
{
  if (JSON.typeof(linActs) != "array")
  {
    return false;
  }

  if ((int)linActs.length() != ACTUATOR_COUNT)
  {
    return false;
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    JSONVar value = linActs[i];
    String valueType = JSON.typeof(value);
    if (valueType != "number")
    {
      return false;
    }

    double target = (double)value;
    if (target < 0.0 || target > 1023.0)
    {
      return false;
    }

    parsedTargets[i] = (int)target;
  }

  return true;
}

void processMessageLine(const char *line)
{
  if (line[0] == '\0')
  {
    sendError(-1, "empty_message");
    return;
  }

  JSONVar message = JSON.parse(line);
  if (JSON.typeof(message) == "undefined")
  {
    sendError(-1, "invalid_json");
    return;
  }

  if (!message.hasOwnProperty("id"))
  {
    sendError(-1, "invalid_id");
    return;
  }

  JSONVar idValue = message["id"];
  if (JSON.typeof(idValue) != "number")
  {
    sendError(-1, "invalid_id");
    return;
  }

  long messageId = (long)((double)idValue);

  if (!message.hasOwnProperty("lin_acts"))
  {
    sendError(messageId, "missing_lin_acts");
    return;
  }

  int nextTargets[ACTUATOR_COUNT];
  JSONVar linActs = message["lin_acts"];
  if (!parseTargetsFromJson(linActs, nextTargets))
  {
    sendError(messageId, "invalid_lin_acts");
    return;
  }

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetValues[i] = nextTargets[i];
  }

  lastCommandId = messageId;
  sendTelemetry(lastCommandId);
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
        sendError(-1, "input_overflow");
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
  sendTelemetry(lastCommandId);
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
    sendTelemetry(lastCommandId);
  }
}
