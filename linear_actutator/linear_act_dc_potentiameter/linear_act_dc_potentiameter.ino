/*
IBT-2 Motor Control Board driven by Arduino.
 
Speed and direction controlled by a potentiometer attached to analog input 0.
One side pin of the potentiometer (either one) to ground; the other side pin to +5V
 
Connection to the IBT-2 board:
IBT-2 pin 1 (RPWM) to Arduino pin 5(PWM)
IBT-2 pin 2 (LPWM) to Arduino pin 6(PWM)
IBT-2 pins 3 (R_EN), 4 (L_EN), 7 (VCC) to Arduino 5V pin
IBT-2 pin 8 (GND) to Arduino GND
IBT-2 pins 5 (R_IS) and 6 (L_IS) not connected
*/
 
const int SENSOR_PIN = A0;

const int RPWM_Output = 5;
const int LPWM_Output = 6;

const int DIR_BACKWARD = -1;
const int DIR_STOP = 0;
const int DIR_FORWARD = 1;

// Set true if forward/backward are swapped for your wiring/mechanics.
const bool invertDirection = false;

int motorDirection = DIR_STOP;
bool hasValidDirectionCmd = false;

char serialBuffer[12];
byte serialIndex = 0;

void driveMotor(int direction, int pwm)
{
  if (!hasValidDirectionCmd || direction == DIR_STOP)
  {
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output, 0);
    return;
  }

  bool driveRPWM = (direction == DIR_FORWARD);
  if (invertDirection)
  {
    driveRPWM = !driveRPWM;
  }

  if (driveRPWM)
  {
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, pwm);
  }
  else
  {
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output, pwm);
  }
}

void applyDirectionLine(char *line)
{
  while (*line == ' ' || *line == '\t')
  {
    line++;
  }

  int len = strlen(line);
  while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
  {
    line[len - 1] = '\0';
    len--;
  }

  if (strcmp(line, "-1") == 0)
  {
    motorDirection = DIR_BACKWARD;
    hasValidDirectionCmd = true;
    Serial.println("OK -1");
  }
  else if (strcmp(line, "0") == 0)
  {
    motorDirection = DIR_STOP;
    hasValidDirectionCmd = true;
    Serial.println("OK 0");
  }
  else if (strcmp(line, "1") == 0)
  {
    motorDirection = DIR_FORWARD;
    hasValidDirectionCmd = true;
    Serial.println("OK 1");
  }
  else if (len > 0)
  {
    Serial.println("ERR");
  }
}

void readSerialDirection()
{
  while (Serial.available() > 0)
  {
    char c = (char)Serial.read();

    if (c == '\r')
    {
      continue;
    }

    if (c == '\n')
    {
      serialBuffer[serialIndex] = '\0';
      if (serialIndex > 0)
      {
        applyDirectionLine(serialBuffer);
      }
      serialIndex = 0;
      continue;
    }

    if (serialIndex < sizeof(serialBuffer) - 1)
    {
      serialBuffer[serialIndex++] = c;
    }
    else
    {
      serialIndex = 0;
      Serial.println("ERR");
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(RPWM_Output, OUTPUT);
  pinMode(LPWM_Output, OUTPUT);

  // Safe startup: always stopped until a valid serial command arrives.
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output, 0);

  Serial.println("READY");
}

void loop()
{
  readSerialDirection();

  int sensorValue = analogRead(SENSOR_PIN);
  int speedPWM = map(sensorValue, 0, 1023, 0, 255);
  speedPWM = constrain(speedPWM, 0, 255);

  driveMotor(motorDirection, speedPWM);
}
