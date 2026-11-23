#pragma once

#include <stdint.h>

// =====================================================
// SERVO IDENTITY + LIMITS
// =====================================================

constexpr int SERVO_HEAD       = 0;
constexpr int SERVO_NECK       = 1;
constexpr int SERVO_HAND_LEFT  = 2;
constexpr int SERVO_HAND_RIGHT = 3;
constexpr int SERVO_BODY       = 4;

constexpr int SERVO_COUNT = 5;

struct ServoSpec {
  const char* name;
  int channel;
  float min;
  float max;
};

// Empty-NVS / stock defaults: narrow band around electrical 90° until setup AP calibration.
constexpr ServoSpec SERVO_SPECS[SERVO_COUNT] = {
  {"HEAD",       SERVO_HEAD,       80.0f, 100.0f},
  {"NECK",       SERVO_NECK,       80.0f, 100.0f},
  {"HAND_LEFT",  SERVO_HAND_LEFT,  80.0f, 100.0f},
  {"HAND_RIGHT", SERVO_HAND_RIGHT, 80.0f, 100.0f},
  {"BODY",       SERVO_BODY,       80.0f, 100.0f},
};

constexpr float servoMid(const ServoSpec& spec) {
  return (spec.min + spec.max) * 0.5f;
}

// Pose space: -1 = saved min, 0 = mid, +1 = saved max.
inline float servoSaturateNorm(float n) {
  if (n < -1.0f) {
    return -1.0f;
  }
  if (n > 1.0f) {
    return 1.0f;
  }
  return n;
}

inline float servoNormToDeg(float n, float savedMin, float savedMax) {
  n = servoSaturateNorm(n);
  return (savedMin + savedMax) * 0.5f + n * (savedMax - savedMin) * 0.5f;
}

inline float servoDegToNorm(float deg, float savedMin, float savedMax) {
  const float span = savedMax - savedMin;
  if (span <= 0.0f) {
    return 0.0f;
  }
  return servoSaturateNorm(2.0f * (deg - savedMin) / span - 1.0f);
}

// Max commanded slew rate for all smooth moves
constexpr float SERVO_MAX_SPEED_DEG_S = 140.0f;

// Setup-wizard calibration moves (horn 90° and per-joint sweeps)
constexpr float SERVO_CALIB_SPEED_DEG_S = 25.0f;

// ~half a PCA9685 count at 800-2200 us over 0-180 deg
constexpr float SERVO_ANGLE_DEADBAND_DEG = 0.32f;

// Approximate angular spacing of one PWM count
constexpr float SERVO_PWM_STEP_DEG = 0.63f;

// Idle/sleep: hold PWM this long after all joints still, then full-off
constexpr uint32_t SERVO_PWM_RELEASE_DELAY_MS = 2000;

// Longest gap one slew step may cover. A loop stalled by a blocking call
// (a smooth or calibration move, a flash write) must not turn into one
// jump of speed x stall time on a joint that is still slewing.
constexpr uint32_t SERVO_MAX_STEP_GAP_MS = 100;

// Next angle of a joint slewing from angle to target at speedDegS,
// elapsedMs after its previous step.
inline float servoSlewNext(
  float angle,
  float target,
  float speedDegS,
  uint32_t elapsedMs
) {
  if (elapsedMs > SERVO_MAX_STEP_GAP_MS) {
    elapsedMs = SERVO_MAX_STEP_GAP_MS;
  }

  const float maxStep = speedDegS * ((float)elapsedMs / 1000.0f);
  const float delta = target - angle;

  if (delta > maxStep) {
    return angle + maxStep;
  }
  if (delta < -maxStep) {
    return angle - maxStep;
  }
  return target;
}
