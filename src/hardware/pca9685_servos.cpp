#include <Arduino.h>

#include "pins.h"
#include "servos.h"
#include "display/oled.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "hardware/pca9685_servos.h"
#include "serial_log.h"

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

    setRgb(64, 0, 0);

    while (true) {
      delay(1000);
    }
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

void centerAllServos() {
  serialLogPrintln("Centering servos to mid (min+max)/2");

  float targets[SERVO_COUNT];

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    targets[servo] = servoMid(SERVO_SPECS[servo]);
  }

  servoMoveAllSmoothTo(targets);
}

void runServoTest() {
  constexpr int TOTAL_STEPS = 4;

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("SERVO TEST - HEAD/NECK/HANDS/BODY");
  serialLogPrintln("==========================");

  serialLogPrintln(
    "1. All servos -> 90 deg"
  );

  showServoProgress(
    1,
    TOTAL_STEPS,
    "ALL -> 90"
  );

  servoMoveAllSmooth(
    SERVO_CENTER
  );

  delay(1000);

  serialLogPrintln(
    "2. All servos: 90 -> 105 deg"
  );

  showServoProgress(
    2,
    TOTAL_STEPS,
    "90 -> 105"
  );

  servoMoveAllSmooth(
    SERVO_HIGH
  );

  serialLogPrintln(
    "3. All servos: 105 -> 75 deg"
  );

  showServoProgress(
    3,
    TOTAL_STEPS,
    "105 -> 75"
  );

  servoMoveAllSmooth(
    SERVO_LOW
  );

  serialLogPrintln(
    "4. All servos: 75 -> 90 deg"
  );

  showServoProgress(
    4,
    TOTAL_STEPS,
    "75 -> 90"
  );

  servoMoveAllSmooth(
    SERVO_CENTER
  );

  showServoTestFinished();

  serialLogPrintln(
    "All 5 servos test finished"
  );

  delay(1000);
}
