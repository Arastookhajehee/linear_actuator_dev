/*
Simple A3 analog read sketch.
Reads analog pin A3 and prints the value to Serial Monitor.
*/

const int ANALOG_PIN = A3;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int value = analogRead(ANALOG_PIN);
  Serial.println(value);
  delay(100);
}
