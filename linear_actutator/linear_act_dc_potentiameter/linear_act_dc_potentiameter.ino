/*
IBT-2 Motor Control Board driven by Arduino.

The actuator follows a target position from a draw-wire sensor:
- Sensor input on A3
- Hard-coded target value = 250
- Sensor sampled every 100 ms
*/

const int SENSOR_PIN = A3;
const int RPWM_Output = 5;
const int LPWM_Output = 6;

const int TARGET_VALUE = 250;
const int TARGET_DEADBAND = 30;
const int DRIVE_PWM = 70;
const bool invertDirection = false;

const unsigned long SAMPLE_INTERVAL_MS = 100;
unsigned long lastSampleMs = 0;

int lastSensorValue = 0;

const int MEDIAN_SAMPLES = 7;

int readMedianSensor()
{
  int samples[MEDIAN_SAMPLES];

  for (int i = 0; i < MEDIAN_SAMPLES; i++)
  {
    samples[i] = analogRead(SENSOR_PIN);
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

void stopMotor()
{
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, 0);
}

void driveTowardTarget(int sensorValue)
{
  int error = TARGET_VALUE - sensorValue;

  if (abs(error) <= TARGET_DEADBAND)
  {
    stopMotor();
    return;
  }

  bool shouldDriveForward = (error > 0);
  if (invertDirection)
  {
    shouldDriveForward = !shouldDriveForward;
  }

  if (shouldDriveForward)
  {
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, DRIVE_PWM);
  }
  else
  {
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output, DRIVE_PWM);
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(RPWM_Output, OUTPUT);
  pinMode(LPWM_Output, OUTPUT);

  stopMotor();
  Serial.println("READY");
}

void loop()
{
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS)
  {
    return;
  }

  lastSampleMs = now;
  lastSensorValue = readMedianSensor();

  driveTowardTarget(lastSensorValue);

  Serial.print("median=");
  Serial.print(lastSensorValue);
  Serial.print(" target=");
  Serial.print(TARGET_VALUE);
  Serial.print(" error=");
  Serial.println(TARGET_VALUE - lastSensorValue);
}
