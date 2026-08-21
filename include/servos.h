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
  {"HAND_LEFT",  SERVO_HAND_LEFT,  50.0f, 140.0f},
  {"HAND_RIGHT", SERVO_HAND_RIGHT, 40.0f, 130.0f},
  {"BODY",       SERVO_BODY,       40.0f, 130.0f},
};

constexpr float servoMid(const ServoSpec& spec) {
  return (spec.min + spec.max) * 0.5f;
}

// Shared angles for /test/movement (not per-servo limits)
constexpr float SERVO_LOW    = 75.0f;
constexpr float SERVO_CENTER = 90.0f;
constexpr float SERVO_HIGH   = 105.0f;

// Max commanded slew rate for all smooth moves
constexpr float SERVO_MAX_SPEED_DEG_S = 220.0f;
