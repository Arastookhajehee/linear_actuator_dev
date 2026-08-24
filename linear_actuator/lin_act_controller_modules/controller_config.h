#ifndef CONTROLLER_CONFIG_H
#define CONTROLLER_CONFIG_H

const int ACTUATOR_COUNT = 4;

const int SENSOR_PINS[ACTUATOR_COUNT] = {A0, A1, A2, A3};
const int RPWM_PINS[ACTUATOR_COUNT] = {2, 4, 6, 8};
const int LPWM_PINS[ACTUATOR_COUNT] = {3, 5, 7, 9};
const bool ACTUATOR_ENABLED[ACTUATOR_COUNT] = {true, true, true, true};
const bool INVERT_DIRECTION[ACTUATOR_COUNT] = {false, false, false, false};

const int DEFAULT_TARGET_MM = 50;
const int TARGET_DEADBAND_MM = 2;
const int PID_START_ERROR_MM = 10;
const int DRIVE_PWM = 200;
const int MIN_PID_PWM = 20;
const int CALIBRATION_PWM = 200;
const unsigned long CALIBRATION_DURATION_MS = 2000;
const unsigned long CALIBRATION_DELAY_MS = 200;
const int CALIBRATION_SENSOR_DELTA_THRESHOLD = 2;
const unsigned long STALL_TIMEOUT_MS = 3000;
const int STALL_RAW_DELTA_THRESHOLD = 2;
const bool STOP_ON_STALL = false;
const int MODE_FILTER_WINDOW = 5;
const float PID_KP = 12.0f;
const float PID_KI = 0.4f;
const float PID_KD = 3.0f;

const unsigned long SAMPLE_INTERVAL_MS = 100;
const unsigned long CALIBRATION_SAMPLE_INTERVAL_MS = 20;
const unsigned long TELEMETRY_INTERVAL_MS = 1000;

const int SERIAL_BUFFER_LEN = 256;

const int CAL_RAW_AT_0 = 3;
const int CAL_RAW_AT_800 = 812;
const bool CAL_RPWM_IS_EXTEND = true;
const bool CAL_ZERO_IS_LOW_SENSOR = true;

const bool DEBUG_SERIAL = true;

#endif
