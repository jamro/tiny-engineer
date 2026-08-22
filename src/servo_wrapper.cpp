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

  const float counts =
    pulseUs * 4096.0f / 20000.0f + 0.5f;

  return (uint16_t)constrain(counts, 0.0f, 4095.0f);
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
    angle_(servoMid(SERVO_SPECS[index])),
    target_(angle_),
    speedDegS_(SERVO_MAX_SPEED_DEG_S),
    lastUpdateMs_(0),
    lastPulse_(UINT16_MAX) {}

float ServoWrapper::angle() const {
  return angle_;
}

void ServoWrapper::setTarget(float target, float speedDegS) {
  target_ = clampServoAngle(index_, target);
  speedDegS_ = constrain(
    speedDegS,
    0.0f,
    SERVO_MAX_SPEED_DEG_S
  );
}

void ServoWrapper::setPosition(float angle) {
  angle = clampServoAngle(index_, angle);
  writeAngle(angle, false);
  target_ = angle_;
  lastUpdateMs_ = 0;
}

void ServoWrapper::stop() {
  target_ = angle_;
}

bool ServoWrapper::isMoving() const {
  return fabsf(target_ - angle_) >= SERVO_ANGLE_DEADBAND_DEG;
}

void ServoWrapper::update() {
  if (!isMoving()) {
    return;
  }

  const uint32_t now = millis();

  if (lastUpdateMs_ == 0) {
    lastUpdateMs_ = now;
    return;
  }

  const uint32_t elapsed = now - lastUpdateMs_;

  if (elapsed < (uint32_t)SERVO_STEP_MS) {
    return;
  }

  lastUpdateMs_ = now;

  const float maxStep =
    speedDegS_ * ((float)elapsed / 1000.0f);
  const float delta = target_ - angle_;
  const float step =
    constrain(delta, -maxStep, maxStep);
  const float next = angle_ + step;

  if (fabsf(target_ - next) < SERVO_ANGLE_DEADBAND_DEG) {
    writeAngle(target_, false);
    lastUpdateMs_ = 0;
    return;
  }

  writeAngle(next, false);
}

void updateAllServos() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].update();
  }
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

  if (pulse != lastPulse_) {
    pwm.setPWM(
      SERVO_SPECS[index_].channel,
      0,
      pulse
    );
    lastPulse_ = pulse;
  }

  angle_ = angle;
}

void ServoWrapper::snapTo(float angle) {
  writeAngle(angle, true);
  target_ = angle_;
  lastUpdateMs_ = 0;
}

bool ServoWrapper::moveTo(float target) {
  target = clampServoAngle(index_, target);
  stop();

  const float fromAngle = angle_;
  const float delta = fabsf(target - fromAngle);

  if (delta < SERVO_ANGLE_DEADBAND_DEG) {
    writeAngle(target, true);
    target_ = angle_;
    lastUpdateMs_ = 0;
    return true;
  }

  const int durationMs =
    (int)((delta / SERVO_MAX_SPEED_DEG_S) * 1000.0f);

  const int minSteps =
    (int)(delta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

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
    delay(stepDelayMs);
  }

  writeAngle(target, true);
  target_ = angle_;
  lastUpdateMs_ = 0;
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

  if (maxDelta < SERVO_ANGLE_DEADBAND_DEG) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      g_servos[i].writeAngle(targets[i], true);
      g_servos[i].target_ = g_servos[i].angle_;
      g_servos[i].lastUpdateMs_ = 0;
    }
    return;
  }

  const int durationMs =
    (int)((maxDelta / SERVO_MAX_SPEED_DEG_S) * 1000.0f);

  const int minSteps =
    (int)(maxDelta / SERVO_PWM_STEP_DEG) + 1;
  const int steps = max(
    max(1, durationMs / SERVO_STEP_MS),
    minSteps
  );
  const int stepDelayMs = max(1, durationMs / steps);

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

    delay(stepDelayMs);
  }

  for (int i = 0; i < SERVO_COUNT; i++) {
    g_servos[i].writeAngle(targets[i], true);
    g_servos[i].target_ = g_servos[i].angle_;
    g_servos[i].lastUpdateMs_ = 0;
  }
}
