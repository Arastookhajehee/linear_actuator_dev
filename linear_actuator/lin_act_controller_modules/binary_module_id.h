#ifndef BINARY_MODULE_ID_H
#define BINARY_MODULE_ID_H

const int BINARY_ID_PIN_COUNT = 4;
const int BINARY_ID_PINS[BINARY_ID_PIN_COUNT] = {23, 25, 27, 29};

void updateBinaryModuleId()
{
  int value = 0;

  for (int i = 0; i < BINARY_ID_PIN_COUNT; i++)
  {
    int bit = digitalRead(BINARY_ID_PINS[i]) == HIGH ? 1 : 0;
    binaryModuleIdBits[i] = bit;
    value = (value << 1) | bit;
  }

  binaryModuleIdValue = value;
}

void setupBinaryModuleIdPins()
{
  for (int i = 0; i < BINARY_ID_PIN_COUNT; i++)
  {
    pinMode(BINARY_ID_PINS[i], INPUT);
  }

  updateBinaryModuleId();
}

#endif
