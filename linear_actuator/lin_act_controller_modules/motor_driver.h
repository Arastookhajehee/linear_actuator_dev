#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

bool isActuatorConfigured(int actuatorIndex)
{
  return ACTUATOR_ENABLED[actuatorIndex] && sensorConnected[actuatorIndex];
}

void stopMotor(int actuatorIndex)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    return;
  }

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

void driveActuator(int actuatorIndex, bool useRPWM, int pwm)
{
  if (!isActuatorConfigured(actuatorIndex))
  {
    return;
  }

  if (useRPWM)
  {
    analogWrite(LPWM_PINS[actuatorIndex], 0);
    analogWrite(RPWM_PINS[actuatorIndex], pwm);
    return;
  }

  analogWrite(RPWM_PINS[actuatorIndex], 0);
  analogWrite(LPWM_PINS[actuatorIndex], pwm);
}

#endif
