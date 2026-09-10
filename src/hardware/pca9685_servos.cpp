#include <Arduino.h>

#include "pins.h"
#include "servos.h"
#include "display/oled.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "hardware/pca9685_servos.h"
#include "serial_log.h"
#include "settings/settings.h"

bool pca9685Connected() {
  return i2cDeviceConnected(PCA9685_ADDRESS);
}

void initPca9685() {
  serialLogPrintln();
  serialLogPrintln("Checking PCA9685 at 0x40...");

  if (!pca9685Connected()) {
    serialLogPrintln("ERROR: PCA9685 not found");

    showOledText(
      "PCA9685 ERROR",
      "Not found"
    );

    haltWithRgbCode(RGB_CODE_PCA9685);
  }

  serialLogPrintln("PCA9685 found");

  initServoPwmDriver();
}

bool moveServoSmooth(int index, float toAngle) {
  if (index < 0 || index >= SERVO_COUNT) {
    return false;
  }

  return servoAt(index).moveTo(toAngle);
}

bool moveServoSmoothElectrical(int index, float toAngle) {
  if (index < 0 || index >= SERVO_COUNT) {
    return false;
  }

  return servoAt(index).moveToElectrical(toAngle, SERVO_CALIB_SPEED_DEG_S);
}

void servoMoveAllToElectricalAngle(float angle) {
  float targets[SERVO_COUNT];

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    targets[servo] = angle;
  }

  servoMoveAllToElectrical(targets, SERVO_CALIB_SPEED_DEG_S);
}

void centerAllServos() {
  serialLogPrintln("Centering servos to mid (min+max)/2");

  float targets[SERVO_COUNT];

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    targets[servo] = servoNormToDeg(servo, 0.0f);
  }

  servoMoveAllSmoothTo(targets);
}

void servoMoveAllNorm(float n) {
  float targets[SERVO_COUNT];

  for (int i = 0; i < SERVO_COUNT; i++) {
    targets[i] = servoNormToDeg(i, n);
  }

  servoMoveAllSmoothTo(targets, SERVO_MAX_SPEED_DEG_S);
}

void runServoTest() {
  constexpr int TOTAL_STEPS = 4;

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("SERVO TEST - HEAD/NECK/HANDS/BODY");
  serialLogPrintln("==========================");

  serialLogPrintln("1. All servos -> mid");

  showServoProgress(
    1,
    TOTAL_STEPS,
    "ALL -> mid"
  );

  servoMoveAllNorm(0.0f);

  delay(1000);

  serialLogPrintln("2. All servos: mid -> +0.5");

  showServoProgress(
    2,
    TOTAL_STEPS,
    "mid -> +0.5"
  );

  servoMoveAllNorm(0.5f);

  serialLogPrintln("3. All servos: +0.5 -> -0.5");

  showServoProgress(
    3,
    TOTAL_STEPS,
    "+0.5 -> -0.5"
  );

  servoMoveAllNorm(-0.5f);

  serialLogPrintln("4. All servos: -0.5 -> mid");

  showServoProgress(
    4,
    TOTAL_STEPS,
    "-0.5 -> mid"
  );

  servoMoveAllNorm(0.0f);

  showServoTestFinished();

  serialLogPrintln(
    "All 5 servos test finished"
  );

  delay(1000);
}
