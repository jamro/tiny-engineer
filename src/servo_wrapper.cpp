#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#include "pins.h"
#include "servos.h"
#include "servo_wrapper.h"

Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);

ServoWrapper g_servos[SERVO_COUNT] = {
  ServoWrapper(SERVO_HEAD),
  ServoWrapper(SERVO_NECK),
  ServoWrapper(SERVO_HAND_LEFT),
  ServoWrapper(SERVO_HAND_RIGHT),
  ServoWrapper(SERVO_BODY)
};

uint16_t angleToPulse(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);

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

float clampServoAngle(int index, float angle) {
  const ServoSpec& spec = SERVO_SPECS[index];
  return constrain(angle, spec.min, spec.max);
}

void initServoPwmDriver() {
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);
}

ServoWrapper& servoAt(int index) {
  return g_servos[index];
}

ServoWrapper::ServoWrapper(int index)
  : index_(index),
    angle_(servoMid(SERVO_SPECS[index])) {}

float ServoWrapper::angle() const {
  return angle_;
}

void ServoWrapper::writeAngle(float angle, bool log) {
  angle = clampServoAngle(index_, angle);

  const uint16_t pulse = angleToPulse(angle);

  if (log) {
    Serial.print("Servo ");
    Serial.print(SERVO_SPECS[index_].name);
    Serial.print(" -> ");
    Serial.print(angle);
    Serial.print(" deg, pulse: ");
    Serial.println(pulse);
  }

  pwm.setPWM(
    SERVO_SPECS[index_].channel,
    0,
    pulse
  );

  angle_ = angle;
}

void ServoWrapper::snapTo(float angle) {
  writeAngle(angle, true);
}

bool ServoWrapper::moveTo(float target) {
  target = clampServoAngle(index_, target);

  const float fromAngle = angle_;
  const float delta = fabsf(target - fromAngle);

  if (delta < 0.01f) {
    writeAngle(target, true);
    return true;
  }

  const int durationMs =
    (int)((delta / SERVO_MAX_SPEED_DEG_S) * 1000.0f);

  const int steps =
    max(1, durationMs / SERVO_STEP_MS);

  Serial.print("Servo ");
  Serial.print(SERVO_SPECS[index_].name);
  Serial.print(" smooth ");
  Serial.print(fromAngle);
  Serial.print(" -> ");
  Serial.print(target);
  Serial.print(" deg (");
  Serial.print(SERVO_MAX_SPEED_DEG_S);
  Serial.println(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      (float)step / (float)steps;

    const float angle =
      fromAngle +
      (target - fromAngle) *
      progress;

    writeAngle(angle, false);
    delay(SERVO_STEP_MS);
  }

  writeAngle(target, true);
  return true;
}

void servoMoveAllSmooth(float toAngle) {
  float fromAngles[SERVO_COUNT];
  float targets[SERVO_COUNT];
  float maxDelta = 0.0f;

  for (int i = 0; i < SERVO_COUNT; i++) {
    fromAngles[i] = g_servos[i].angle_;
    targets[i] = clampServoAngle(i, toAngle);
    maxDelta = max(
      maxDelta,
      fabsf(targets[i] - fromAngles[i])
    );
  }

  if (maxDelta < 0.01f) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      g_servos[i].writeAngle(targets[i], true);
    }
    return;
  }

  const int durationMs =
    (int)((maxDelta / SERVO_MAX_SPEED_DEG_S) * 1000.0f);

  const int steps =
    max(1, durationMs / SERVO_STEP_MS);

  Serial.print("All servos smooth -> ");
  Serial.print(toAngle);
  Serial.print(" deg (");
  Serial.print(SERVO_MAX_SPEED_DEG_S);
  Serial.println(" deg/s)");

  for (int step = 1; step <= steps; step++) {
    const float progress =
      (float)step / (float)steps;

    for (int i = 0; i < SERVO_COUNT; i++) {
      const float angle =
        fromAngles[i] +
        (targets[i] - fromAngles[i]) *
        progress;

      g_servos[i].writeAngle(angle, false);
    }

    delay(SERVO_STEP_MS);
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].writeAngle(targets[i], true);
  }
}
