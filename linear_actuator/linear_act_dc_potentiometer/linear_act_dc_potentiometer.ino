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

const int DEFAULT_TARGET = 50;
const int TARGET_DEADBAND = 10;
const int DRIVE_PWM = 70;
const int MEDIAN_SAMPLES = 7;

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
    return;
  }

  analogWrite(RPWM_PINS[actuatorIndex], 0);
  analogWrite(LPWM_PINS[actuatorIndex], DRIVE_PWM);
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
