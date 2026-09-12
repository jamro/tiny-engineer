#pragma once

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

constexpr ServoSpec SERVO_SPECS[SERVO_COUNT] = {
  {"HEAD",       SERVO_HEAD,       60.0f, 130.0f},
  {"NECK",       SERVO_NECK,       40.0f, 130.0f},
  {"HAND_LEFT",  SERVO_HAND_LEFT,  45.0f, 135.0f},
  {"HAND_RIGHT", SERVO_HAND_RIGHT, 35.0f, 125.0f},
  {"BODY",       SERVO_BODY,       40.0f, 130.0f},
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
