#ifndef SENSOR_POSITION_H
#define SENSOR_POSITION_H

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

#endif
