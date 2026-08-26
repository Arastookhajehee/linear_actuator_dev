#ifndef CALIBRATION_ROUTINES_H
#define CALIBRATION_ROUTINES_H

void finishActuatorCalibration(int actuatorIndex)
{
  int rpwmTrend = calibrationRpwmTrend[actuatorIndex];
  int lpwmTrend = calibrationLpwmTrend[actuatorIndex];
  bool sensorResponds = true;
  bool extendsWithRPWM = CAL_RPWM_IS_EXTEND;

  if (abs(rpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    bool rpwmIncreasesSensor = (rpwmTrend > 0);
    extendsWithRPWM = CAL_ZERO_IS_LOW_SENSOR ? rpwmIncreasesSensor : !rpwmIncreasesSensor;
  }
  else if (abs(lpwmTrend) >= CALIBRATION_SENSOR_DELTA_THRESHOLD)
  {
    bool lpwmIncreasesSensor = (lpwmTrend > 0);
    bool lpwmIsExtend = CAL_ZERO_IS_LOW_SENSOR ? lpwmIncreasesSensor : !lpwmIncreasesSensor;
    extendsWithRPWM = !lpwmIsExtend;
  }
  else
  {
    sensorResponds = false;
  }

  rpwmIsExtend[actuatorIndex] = extendsWithRPWM;
  calibrationPhase[actuatorIndex] = CAL_DONE;

  if (!sensorResponds)
  {
    sensorConnected[actuatorIndex] = false;
    calibrationSensorFailure = true;
    Serial.print("DBG sensor ");
    Serial.print(actuatorIndex);
    Serial.println(" not responding during calibration -> disabled");
    return;
  }

  if (DEBUG_SERIAL)
  {
    Serial.print("DBG cal actuator ");
    Serial.print(actuatorIndex);
    Serial.print(" rpwm_trend=");
    Serial.print(rpwmTrend);
    Serial.print(" lpwm_trend=");
    Serial.print(lpwmTrend);
    Serial.print(" rpwm_is_extend=");
    Serial.println(extendsWithRPWM ? 1 : 0);
  }
}

void startActuatorDirectionCalibration()
{
  debugLog("starting concurrent direction calibration");
  stopAllMotors();
  unsigned long now = millis();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    if (!isActuatorConfigured(i))
    {
      calibrationPhase[i] = CAL_DONE;
      continue;
    }

    int sample = readModeFilteredSensor(i);
    calibrationStartSample[i] = sample;
    calibrationLastSample[i] = sample;
    calibrationPhaseStartMs[i] = now;
    calibrationLastSampleMs[i] = now;
    calibrationPhase[i] = CAL_DRIVE_RPWM;
    driveActuator(i, true, CALIBRATION_PWM);
  }
}

void updateActuatorDirectionCalibration()
{
  if (calibrationComplete)
  {
    return;
  }

  unsigned long now = millis();
  bool allDone = true;

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    CalibrationPhase phase = calibrationPhase[i];
    if (phase == CAL_DONE)
    {
      continue;
    }

    allDone = false;
    if (phase == CAL_DRIVE_RPWM || phase == CAL_DRIVE_LPWM)
    {
      if (now - calibrationLastSampleMs[i] >= CALIBRATION_SAMPLE_INTERVAL_MS)
      {
        calibrationLastSampleMs[i] = now;
        calibrationLastSample[i] = readModeFilteredSensor(i);
      }
    }

    if (phase == CAL_DRIVE_RPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DURATION_MS)
    {
      stopMotor(i);
      calibrationLastSample[i] = readModeFilteredSensor(i);
      calibrationRpwmTrend[i] = calibrationLastSample[i] - calibrationStartSample[i];
      calibrationPhase[i] = CAL_SETTLE_AFTER_RPWM;
      calibrationPhaseStartMs[i] = now;
    }
    else if (phase == CAL_SETTLE_AFTER_RPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DELAY_MS)
    {
      int sample = readModeFilteredSensor(i);
      calibrationStartSample[i] = sample;
      calibrationLastSample[i] = sample;
      calibrationLastSampleMs[i] = now;
      calibrationPhase[i] = CAL_DRIVE_LPWM;
      calibrationPhaseStartMs[i] = now;
      driveActuator(i, false, CALIBRATION_PWM);
    }
    else if (phase == CAL_DRIVE_LPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DURATION_MS)
    {
      stopMotor(i);
      calibrationLastSample[i] = readModeFilteredSensor(i);
      calibrationLpwmTrend[i] = calibrationLastSample[i] - calibrationStartSample[i];
      calibrationPhase[i] = CAL_SETTLE_AFTER_LPWM;
      calibrationPhaseStartMs[i] = now;
    }
    else if (phase == CAL_SETTLE_AFTER_LPWM && now - calibrationPhaseStartMs[i] >= CALIBRATION_DELAY_MS)
    {
      finishActuatorCalibration(i);
    }
  }

  if (!allDone)
  {
    allDone = true;
    for (int i = 0; i < ACTUATOR_COUNT; i++)
    {
      if (calibrationPhase[i] != CAL_DONE)
      {
        allDone = false;
        break;
      }
    }
  }

  if (allDone)
  {
    calibrationComplete = true;
    debugLog("concurrent direction calibration complete");
  }
}

#endif
