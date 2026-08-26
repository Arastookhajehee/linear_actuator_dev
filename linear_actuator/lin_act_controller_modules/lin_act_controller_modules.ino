#include <Arduino.h>
#include <Arduino_JSON.h>
#include <stdlib.h>

#include "controller_config.h"
#include "controller_state.h"
#include "debug_helpers.h"
#include "binary_module_id.h"
#include "motor_driver.h"
#include "sensor_position.h"
#include "pid_control.h"
#include "serial_protocol.h"
#include "calibration_routines.h"
#include "motion_routines.h"
#include "runtime_routines.h"

void setup()
{
  Serial.begin(9600);
  debugLog("setup start");
  setupBinaryModuleIdPins();

  for (int i = 0; i < ACTUATOR_COUNT; i++)
  {
    pinMode(RPWM_PINS[i], OUTPUT);
    pinMode(LPWM_PINS[i], OUTPUT);
  }

  stopAllMotors();
  startActuatorDirectionCalibration();
  debugLog("setup initialized");
}

void loop()
{
  updateBinaryModuleId();
  handleSerialInput();

  unsigned long now = millis();
  runCalibrationStep();
  runMotionStep(now);
  runTelemetryStep(now);
}
