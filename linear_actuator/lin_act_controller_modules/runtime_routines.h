#ifndef RUNTIME_ROUTINES_H
#define RUNTIME_ROUTINES_H

void runCalibrationStep()
{
  if (calibrationComplete)
  {
    return;
  }

  updateActuatorDirectionCalibration();
  if (calibrationComplete && !initialMoveStarted)
  {
    initialMoveStarted = true;
    updateCurrentMmFromSensors();
    sendTelemetry();
    lastTelemetryMs = millis();
    if (calibrationSensorFailure)
    {
      sendError("sensor_not_connected");
    }
    moveAllActuatorsToInitialPosition();
  }
}

void runMotionStep(unsigned long now)
{
  if (calibrationComplete && now - lastSampleMs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleMs = now;
    updateCurrentMmFromSensors();
    if (targetsActive)
    {
      updateActuatorMotion();
    }
  }
}

void runTelemetryStep(unsigned long now)
{
  if (calibrationComplete && now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = now;
    sendTelemetry();
  }
}

#endif
