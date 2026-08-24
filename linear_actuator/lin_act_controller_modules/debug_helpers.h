#ifndef DEBUG_HELPERS_H
#define DEBUG_HELPERS_H

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

#endif
