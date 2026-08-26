#ifndef MOTION_ROUTINES_H
#define MOTION_ROUTINES_H

void sendTelemetry();

void moveAllActuatorsToInitialPosition()
{
  debugLog("starting concurrent move to 50mm");
  updateCurrentMmFromSensors();
  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetMm[i] = DEFAULT_TARGET_MM;
    lastProgressRaw[i] = currentRaw[i];
    lastProgressMs[i] = now;
    stallWarningReported[i] = false;
    resetPidState(i);
    actuatorMotionActive[i] = isActuatorConfigured(i);
  }

  targetsActive = true;
}

void updateActuatorMotion()
{
  if (!targetsActive)
  {
    return;
  }

  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!isActuatorConfigured(i) || !actuatorMotionActive[i])
    {
      continue;
    }

    int error = targetMm[i] - currentMm[i];
    if (abs(error) <= TARGET_DEADBAND_MM)
    {
      stopMotor(i);
      resetPidState(i);
      actuatorMotionActive[i] = false;
      Serial.print("DBG actuator ");
      Serial.print(i);
      Serial.print(" reached target=");
      Serial.println(targetMm[i]);
      continue;
    }

    if (abs(currentRaw[i] - lastProgressRaw[i]) >= STALL_RAW_DELTA_THRESHOLD)
    {
      lastProgressRaw[i] = currentRaw[i];
      lastProgressMs[i] = now;
      stallWarningReported[i] = false;
    }

    if (now - lastProgressMs[i] >= STALL_TIMEOUT_MS)
    {
      if (!stallWarningReported[i])
      {
        Serial.print("DBG actuator ");
        Serial.print(i);
        Serial.print(" stall warning current=");
        Serial.print(currentMm[i]);
        Serial.print(" target=");
        Serial.println(targetMm[i]);
        stallWarningReported[i] = true;
      }

      if (STOP_ON_STALL)
      {
        stopMotor(i);
        resetPidState(i);
        actuatorMotionActive[i] = false;
        continue;
      }
    }

    applyActuatorControl(i, error);
  }

  bool anyActive = false;
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (isActuatorConfigured(i) && actuatorMotionActive[i])
    {
      anyActive = true;
      break;
    }
  }

  targetsActive = anyActive;
  if (!targetsActive)
  {
    debugLog("all active actuators stopped");
    sendTelemetry();
  }
}

void applyTargets(const int nextTargets[ACTUATOR_COUNT])
{
  unsigned long now = millis();
  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    targetMm[i] = nextTargets[i];
    lastProgressRaw[i] = currentRaw[i];
    lastProgressMs[i] = now;
    stallWarningReported[i] = false;
    resetPidState(i);
    actuatorMotionActive[i] = isActuatorConfigured(i);
  }

  targetsActive = true;
  debugTargets("applied");
}

#endif
