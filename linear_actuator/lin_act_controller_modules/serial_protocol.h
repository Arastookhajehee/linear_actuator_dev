#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

void applyTargets(const int nextTargets[ACTUATOR_COUNT]);

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

    if (endPtr == cursor || parsedValue < 0 || parsedValue > 800)
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

#endif
