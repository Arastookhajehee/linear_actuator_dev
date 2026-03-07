/*
  Linear actuator controller for Arduino Mega.

  Serial protocol is newline-delimited JSON with a flat schema (no arrays):
  {
    "a1_current": <number|null>, "a1_target": <number|null>,
    "a2_current": <number|null>, "a2_target": <number|null>,
    "a3_current": <number|null>, "a3_target": <number|null>,
    "a4_current": <number|null>, "a4_target": <number|null>
  }
*/

#include <Arduino_JSON.h>
#include <stdlib.h>

const int ACTUATOR_COUNT = 4;
const int SENSOR_PINS[ACTUATOR_COUNT] = {A1, A2, A3, A4};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};

const int DEFAULT_TARGET = 150;
const int TARGET_DEADBAND = 30;
const int DRIVE_PWM = 70;
const int MEDIAN_SAMPLES = 7;

const unsigned long SAMPLE_INTERVAL_MS = 100;
const unsigned long TELEMETRY_INTERVAL_MS = 1000;
const int SERIAL_BUFFER_LEN = 256;

const bool DEBUG_SERIAL = true;

struct OptionalNumber {
  bool isNull;
  double value;
};

class ActuatorStateMessage {
public:
  OptionalNumber a1_current;
  OptionalNumber a1_target;
  OptionalNumber a2_current;
  OptionalNumber a2_target;
  OptionalNumber a3_current;
  OptionalNumber a3_target;
  OptionalNumber a4_current;
  OptionalNumber a4_target;

  static ActuatorStateMessage createEmpty()
  {
    ActuatorStateMessage message;
    message.a1_current = nullNumber();
    message.a1_target = nullNumber();
    message.a2_current = nullNumber();
    message.a2_target = nullNumber();
    message.a3_current = nullNumber();
    message.a3_target = nullNumber();
    message.a4_current = nullNumber();
    message.a4_target = nullNumber();
    return message;
  }

  static OptionalNumber nullNumber()
  {
    OptionalNumber number;
    number.isNull = true;
    number.value = 0.0;
    return number;
  }

  static OptionalNumber valueNumber(double value)
  {
    OptionalNumber number;
    number.isNull = false;
    number.value = value;
    return number;
  }
};

class ActuatorController {
public:
  ActuatorController()
  {
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      targetValues[i] = DEFAULT_TARGET;
      currentValues[i] = 0;
    }
  }

  void beginPins()
  {
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      pinMode(RPWM_PINS[i], OUTPUT);
      pinMode(LPWM_PINS[i], OUTPUT);
    }
    stopAllMotors();
  }

  void stopAllMotors()
  {
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      stopMotor(i);
    }
  }

  void sampleAndDrive()
  {
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      currentValues[i] = readMedianSensor(SENSOR_PINS[i]);
      driveTowardTarget(i, currentValues[i]);
    }
  }

  void applyTargets(const ActuatorStateMessage &message)
  {
    applySingleTarget(0, message.a1_target);
    applySingleTarget(1, message.a2_target);
    applySingleTarget(2, message.a3_target);
    applySingleTarget(3, message.a4_target);
  }

  ActuatorStateMessage buildTelemetryMessage() const
  {
    ActuatorStateMessage message = ActuatorStateMessage::createEmpty();

    message.a1_current = ActuatorStateMessage::valueNumber((double)currentValues[0]);
    message.a1_target = ActuatorStateMessage::valueNumber((double)targetValues[0]);

    message.a2_current = ActuatorStateMessage::valueNumber((double)currentValues[1]);
    message.a2_target = ActuatorStateMessage::valueNumber((double)targetValues[1]);

    message.a3_current = ActuatorStateMessage::valueNumber((double)currentValues[2]);
    message.a3_target = ActuatorStateMessage::valueNumber((double)targetValues[2]);

    message.a4_current = ActuatorStateMessage::valueNumber((double)currentValues[3]);
    message.a4_target = ActuatorStateMessage::valueNumber((double)targetValues[3]);

    return message;
  }

  const int *targets() const
  {
    return targetValues;
  }

private:
  int targetValues[ACTUATOR_COUNT];
  int currentValues[ACTUATOR_COUNT];

  void stopMotor(int actuatorIndex)
  {
    analogWrite(RPWM_PINS[actuatorIndex], 0);
    analogWrite(LPWM_PINS[actuatorIndex], 0);
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

  void applySingleTarget(int index, const OptionalNumber &target)
  {
    if (target.isNull)
    {
      return;
    }

    double rawTarget = target.value;
    if (rawTarget < 0.0)
    {
      targetValues[index] = 0;
      return;
    }

    if (rawTarget > 1023.0)
    {
      targetValues[index] = 1023;
      return;
    }

    targetValues[index] = (int)rawTarget;
  }
};

class ActuatorJsonCodec {
public:
  static bool parseIncomingMessage(const char *line, ActuatorStateMessage &messageOut, const char *&errorCode)
  {
    if (line[0] == '\0')
    {
      errorCode = "empty_message";
      return false;
    }

    JSONVar root = JSON.parse(line);
    if (JSON.typeof(root) != "object")
    {
      errorCode = "invalid_json";
      return false;
    }

    ActuatorStateMessage parsed = ActuatorStateMessage::createEmpty();

    if (!parseOptionalNumberField(root, "a1_current", parsed.a1_current))
    {
      errorCode = "invalid_a1_current";
      return false;
    }

    if (!parseOptionalNumberField(root, "a2_current", parsed.a2_current))
    {
      errorCode = "invalid_a2_current";
      return false;
    }

    if (!parseOptionalNumberField(root, "a3_current", parsed.a3_current))
    {
      errorCode = "invalid_a3_current";
      return false;
    }

    if (!parseOptionalNumberField(root, "a4_current", parsed.a4_current))
    {
      errorCode = "invalid_a4_current";
      return false;
    }

    if (!parseOptionalNumberField(root, "a1_target", parsed.a1_target))
    {
      errorCode = "invalid_a1_target";
      return false;
    }

    if (!parseOptionalNumberField(root, "a2_target", parsed.a2_target))
    {
      errorCode = "invalid_a2_target";
      return false;
    }

    if (!parseOptionalNumberField(root, "a3_target", parsed.a3_target))
    {
      errorCode = "invalid_a3_target";
      return false;
    }

    if (!parseOptionalNumberField(root, "a4_target", parsed.a4_target))
    {
      errorCode = "invalid_a4_target";
      return false;
    }

    if (!validateTargetRange(parsed.a1_target) ||
        !validateTargetRange(parsed.a2_target) ||
        !validateTargetRange(parsed.a3_target) ||
        !validateTargetRange(parsed.a4_target))
    {
      errorCode = "target_out_of_range";
      return false;
    }

    messageOut = parsed;
    return true;
  }

  static String serializeTelemetry(const ActuatorStateMessage &message)
  {
    JSONVar root;

    writeOptionalNumberField(root, "a1_current", message.a1_current);
    writeOptionalNumberField(root, "a1_target", message.a1_target);

    writeOptionalNumberField(root, "a2_current", message.a2_current);
    writeOptionalNumberField(root, "a2_target", message.a2_target);

    writeOptionalNumberField(root, "a3_current", message.a3_current);
    writeOptionalNumberField(root, "a3_target", message.a3_target);

    writeOptionalNumberField(root, "a4_current", message.a4_current);
    writeOptionalNumberField(root, "a4_target", message.a4_target);

    return JSON.stringify(root);
  }

  static String serializeError(const char *errorCode)
  {
    JSONVar root;
    root["error"] = errorCode;
    return JSON.stringify(root);
  }

private:
  static bool parseOptionalNumberField(const JSONVar &root, const char *key, OptionalNumber &out)
  {
    if (!root.hasOwnProperty(key))
    {
      return false;
    }

    JSONVar value = root[key];
    String valueText = JSON.stringify(value);

    if (valueText == "null")
    {
      out = ActuatorStateMessage::nullNumber();
      return true;
    }

    char *endPtr;
    double parsed = strtod(valueText.c_str(), &endPtr);

    if (endPtr == valueText.c_str() || *endPtr != '\0')
    {
      return false;
    }

    out = ActuatorStateMessage::valueNumber(parsed);
    return true;
  }

  static void writeOptionalNumberField(JSONVar &root, const char *key, const OptionalNumber &value)
  {
    if (value.isNull)
    {
      root[key] = nullptr;
      return;
    }

    root[key] = value.value;
  }

  static bool validateTargetRange(const OptionalNumber &target)
  {
    if (target.isNull)
    {
      return true;
    }

    return target.value >= 0.0 && target.value <= 1023.0;
  }
};

ActuatorController controller;
char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;
bool serialLineOverflow = false;

unsigned long lastSampleMs = 0;
unsigned long lastTelemetryMs = 0;

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

  const int *targets = controller.targets();
  Serial.print("DBG ");
  Serial.print(label);
  Serial.print(" targets=[");

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    Serial.print(targets[i]);
    if (i < ACTUATOR_COUNT - 1)
    {
      Serial.print(",");
    }
  }

  Serial.println("]");
}

void sendTelemetry()
{
  ActuatorStateMessage telemetry = controller.buildTelemetryMessage();
  String payload = ActuatorJsonCodec::serializeTelemetry(telemetry);
  Serial.println(payload);
}

void sendError(const char *errorCode)
{
  Serial.println(ActuatorJsonCodec::serializeError(errorCode));
  if (DEBUG_SERIAL)
  {
    Serial.print("DBG error=");
    Serial.println(errorCode);
  }
}

void processMessageLine(const char *line)
{
  if (DEBUG_SERIAL)
  {
    Serial.print("DBG received line: ");
    Serial.println(line);
  }

  ActuatorStateMessage message;
  const char *errorCode = "invalid_json";

  if (!ActuatorJsonCodec::parseIncomingMessage(line, message, errorCode))
  {
    sendError(errorCode);
    return;
  }

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG parsed targets=");
    Serial.print(message.a1_target.isNull ? -1 : (int)message.a1_target.value);
    Serial.print(",");
    Serial.print(message.a2_target.isNull ? -1 : (int)message.a2_target.value);
    Serial.print(",");
    Serial.print(message.a3_target.isNull ? -1 : (int)message.a3_target.value);
    Serial.print(",");
    Serial.println(message.a4_target.isNull ? -1 : (int)message.a4_target.value);
  }

  controller.applyTargets(message);
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

  controller.beginPins();
  controller.sampleAndDrive();
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
    controller.sampleAndDrive();
  }

  if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = now;
    sendTelemetry();
  }
}
