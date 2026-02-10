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
 
int SENSOR_PIN = A0; // speed control only
 
int RPWM_Output = 5; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM)
int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM)

int sensorValue = -1;

const int DIR_STOP = 0;
const int DIR_FORWARD = 1;
const int DIR_BACKWARD = -1;

// Set true if forward/backward are swapped for your wiring/mechanics.
const bool invertDirection = false;

// Hard-coded direction: use DIR_FORWARD, DIR_BACKWARD, or DIR_STOP.
const int MOTOR_DIRECTION = DIR_BACKWARD;

void driveMotor(int direction, int pwm)
{
  if (direction == DIR_STOP)
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
 
void setup()
{
  Serial.begin(9600);
  pinMode(RPWM_Output, OUTPUT);
  pinMode(LPWM_Output, OUTPUT);
}
 
void loop()
{
  sensorValue = analogRead(SENSOR_PIN);

  // Use full potentiometer range for speed only.
  int speedPWM = map(sensorValue, 0, 1023, 0, 255);
  speedPWM = constrain(speedPWM, 0, 255);

  driveMotor(MOTOR_DIRECTION, speedPWM);

  Serial.print("pot=");
  Serial.print(sensorValue);
  Serial.print(" pwm=");
  Serial.print(speedPWM);
  Serial.print(" dir=");
  Serial.println(MOTOR_DIRECTION);
}
