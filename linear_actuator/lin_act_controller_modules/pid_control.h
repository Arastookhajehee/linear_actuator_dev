#ifndef PID_CONTROL_H
#define PID_CONTROL_H

void resetPidState(int actuatorIndex)
{
  pidIntegral[actuatorIndex] = 0;
  pidPreviousError[actuatorIndex] = 0;
}

void applyActuatorControl(int actuatorIndex, int error)
{
  if (abs(error) <= TARGET_DEADBAND_MM)
  {
    stopMotor(actuatorIndex);
    resetPidState(actuatorIndex);
    return;
  }

  bool shouldExtend = (error > 0);
  bool useRPWM = rpwmIsExtend[actuatorIndex];
  if (INVERT_DIRECTION[actuatorIndex])
  {
    useRPWM = !useRPWM;
  }
  if (!shouldExtend)
  {
    useRPWM = !useRPWM;
  }

  int pwm = DRIVE_PWM;
  if (abs(error) <= PID_START_ERROR_MM)
  {
    pidIntegral[actuatorIndex] += error;
    pidIntegral[actuatorIndex] = constrain(pidIntegral[actuatorIndex], -50, 50);

    int derivative = error - pidPreviousError[actuatorIndex];
    float pidOutput = (PID_KP * (float)error) + (PID_KI * (float)pidIntegral[actuatorIndex]) + (PID_KD * (float)derivative);
    pidPreviousError[actuatorIndex] = error;

    pwm = (int)constrain(abs(pidOutput), MIN_PID_PWM, DRIVE_PWM);
  }
  else
  {
    resetPidState(actuatorIndex);
  }

  driveActuator(actuatorIndex, useRPWM, pwm);
}

#endif
