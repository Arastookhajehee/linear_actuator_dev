#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

char serialBuffer[SERIAL_BUFFER_LEN];
int serialBufferIndex = 0;
bool serialLineOverflow = false;

bool sensorConnected[ACTUATOR_COUNT] = {true, true, true, true};
int currentMm[ACTUATOR_COUNT] = {0, 0, 0, 0};
int currentRaw[ACTUATOR_COUNT] = {0, 0, 0, 0};
int targetMm[ACTUATOR_COUNT] = {DEFAULT_TARGET_MM, DEFAULT_TARGET_MM, DEFAULT_TARGET_MM, DEFAULT_TARGET_MM};
int lastProgressRaw[ACTUATOR_COUNT] = {0, 0, 0, 0};
unsigned long lastProgressMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
bool stallWarningReported[ACTUATOR_COUNT] = {false, false, false, false};
int pidIntegral[ACTUATOR_COUNT] = {0, 0, 0, 0};
int pidPreviousError[ACTUATOR_COUNT] = {0, 0, 0, 0};
int sensorHistory[ACTUATOR_COUNT][MODE_FILTER_WINDOW];
int sensorHistoryIndex[ACTUATOR_COUNT] = {0, 0, 0, 0};
int sensorHistoryCount[ACTUATOR_COUNT] = {0, 0, 0, 0};
int binaryModuleIdBits[4] = {0, 0, 0, 0};
int binaryModuleIdValue = 0;

bool rpwmIsExtend[ACTUATOR_COUNT] = {false, false, false, false};
bool actuatorMotionActive[ACTUATOR_COUNT] = {false, false, false, false};
bool calibrationComplete = false;
bool calibrationSensorFailure = false;
bool targetsActive = false;
bool initialMoveStarted = false;

enum CalibrationPhase
{
  CAL_IDLE,
  CAL_DRIVE_RPWM,
  CAL_SETTLE_AFTER_RPWM,
  CAL_DRIVE_LPWM,
  CAL_SETTLE_AFTER_LPWM,
  CAL_DONE
};

CalibrationPhase calibrationPhase[ACTUATOR_COUNT] = {CAL_IDLE, CAL_IDLE, CAL_IDLE, CAL_IDLE};
unsigned long calibrationPhaseStartMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
unsigned long calibrationLastSampleMs[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationStartSample[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationLastSample[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationRpwmTrend[ACTUATOR_COUNT] = {0, 0, 0, 0};
int calibrationLpwmTrend[ACTUATOR_COUNT] = {0, 0, 0, 0};

unsigned long lastSampleMs = 0;
unsigned long lastTelemetryMs = 0;

#endif
