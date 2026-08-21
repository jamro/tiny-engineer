#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#include "pins.h"
#include "oled.h"
#include "rgb.h"
#include "pca9685_servos.h"

Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);

float lastServoAngles[SERVO_COUNT] = {
  SERVO_CENTER,
  SERVO_CENTER,
  SERVO_CENTER,
  SERVO_CENTER,
  SERVO_CENTER
};

bool pca9685Connected() {
  return i2cDeviceConnected(PCA9685_ADDRESS);
}

void initPca9685() {
  Serial.println();
  Serial.println("Checking PCA9685 at 0x40...");

  if (!pca9685Connected()) {
    Serial.println("ERROR: PCA9685 not found");

    showOledText(
      "PCA9685 ERROR",
      "Not found"
    );

    setRgb(64, 0, 0);

    while (true) {
      delay(1000);
    }
  }

  Serial.println("PCA9685 found");

  pwm.begin();
  pwm.setPWMFreq(50);

  delay(100);
}

uint16_t angleToPulse(float angle) {
  angle = constrain(
    angle,
    0.0f,
    180.0f
  );

  const float pulseUs =
    SERVO_MIN_US +
    (angle / 180.0f) *
    (SERVO_MAX_US - SERVO_MIN_US);

  return (uint16_t)(
    pulseUs *
    4096.0f /
    20000.0f
  );
}

bool setServoAngle(int index, float angle) {
  if (index < 0 || index >= SERVO_COUNT) {
    return false;
  }

  const uint16_t pulse =
    angleToPulse(angle);

  Serial.print("Servo ");
  Serial.print(index);
  Serial.print(" -> ");
  Serial.print(angle);
  Serial.print(" deg, pulse: ");
  Serial.println(pulse);

  pwm.setPWM(
    SERVO_CHANNELS[index],
    0,
    pulse
  );

  lastServoAngles[index] = angle;
  return true;
}

bool moveServoSmooth(int index, float toAngle) {
  if (index < 0 || index >= SERVO_COUNT) {
    return false;
  }

  const float fromAngle =
    lastServoAngles[index];
  const float delta =
    fabsf(toAngle - fromAngle);

  if (delta < 0.01f) {
    return setServoAngle(index, toAngle);
  }

  const int durationMs =
    (int)(
      (delta / SERVO_SPEED_DEG_S) *
      1000.0f
    );

  const int steps =
    max(
      1,
      durationMs / SERVO_STEP_MS
    );

  Serial.print("Servo ");
  Serial.print(index);
  Serial.print(" smooth ");
  Serial.print(fromAngle);
  Serial.print(" -> ");
  Serial.print(toAngle);
  Serial.print(" deg (");
  Serial.print(SERVO_SPEED_DEG_S);
  Serial.println(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      (float)step / (float)steps;

    const float angle =
      fromAngle +
      (toAngle - fromAngle) *
      progress;

    const uint16_t pulse =
      angleToPulse(angle);

    pwm.setPWM(
      SERVO_CHANNELS[index],
      0,
      pulse
    );

    delay(SERVO_STEP_MS);
  }

  return setServoAngle(index, toAngle);
}

void setAllServoAngles(float angle) {
  const uint16_t pulse =
    angleToPulse(angle);

  Serial.print("All servos -> ");
  Serial.print(angle);
  Serial.print(" deg, pulse: ");
  Serial.println(pulse);

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    pwm.setPWM(
      SERVO_CHANNELS[servo],
      0,
      pulse
    );
    lastServoAngles[servo] = angle;
  }
}

void moveAllServosSmooth(
  float fromAngle,
  float toAngle,
  int durationMs
) {
  const int steps =
    max(
      1,
      durationMs / SERVO_STEP_MS
    );

  for (int step = 1; step <= steps; step++) {
    const float progress =
      (float)step / (float)steps;

    const float angle =
      fromAngle +
      (toAngle - fromAngle) *
      progress;

    const uint16_t pulse =
      angleToPulse(angle);

    for (int servo = 0; servo < SERVO_COUNT; servo++) {
      pwm.setPWM(
        SERVO_CHANNELS[servo],
        0,
        pulse
      );
    }

    delay(SERVO_STEP_MS);
  }

  setAllServoAngles(toAngle);
}

void runServoTest() {
  constexpr int TOTAL_STEPS = 4;

  Serial.println();
  Serial.println("==========================");
  Serial.println("SERVO TEST - CHANNELS 0-4");
  Serial.println("==========================");

  Serial.println(
    "1. All servos -> 90 deg"
  );

  showServoProgress(
    1,
    TOTAL_STEPS,
    "ALL -> 90"
  );

  setAllServoAngles(
    SERVO_CENTER
  );

  delay(1000);

  Serial.println(
    "2. All servos: 90 -> 105 deg"
  );

  showServoProgress(
    2,
    TOTAL_STEPS,
    "90 -> 105"
  );

  moveAllServosSmooth(
    SERVO_CENTER,
    SERVO_HIGH,
    1000
  );

  Serial.println(
    "3. All servos: 105 -> 75 deg"
  );

  showServoProgress(
    3,
    TOTAL_STEPS,
    "105 -> 75"
  );

  moveAllServosSmooth(
    SERVO_HIGH,
    SERVO_LOW,
    2000
  );

  Serial.println(
    "4. All servos: 75 -> 90 deg"
  );

  showServoProgress(
    4,
    TOTAL_STEPS,
    "75 -> 90"
  );

  moveAllServosSmooth(
    SERVO_LOW,
    SERVO_CENTER,
    1000
  );

  showServoTestFinished();

  Serial.println(
    "All 5 servos test finished"
  );

  delay(1000);
}
