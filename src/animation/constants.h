#pragma once

#include "servo_wrapper.h"
#include "servos.h"

namespace anim {

constexpr float TYPING_HAND_BAND_DEG = 15.0f;
constexpr float TYPING_HEAD_BAND_DEG = 10.0f;
constexpr float TYPING_SWAY_DEG = 5.0f;
constexpr float READING_HEAD_BAND_DEG = 10.0f;
constexpr float READING_NECK_SWAY_DEG = 10.0f;

constexpr float TYPING_RIGHT_LOW =
  SERVO_SPECS[SERVO_HAND_RIGHT].min;
constexpr float TYPING_RIGHT_HIGH =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + TYPING_HAND_BAND_DEG;
constexpr float TYPING_LEFT_LOW =
  SERVO_SPECS[SERVO_HAND_LEFT].max - TYPING_HAND_BAND_DEG;
constexpr float TYPING_LEFT_HIGH =
  SERVO_SPECS[SERVO_HAND_LEFT].max;

constexpr float TYPING_HEAD_LOW =
  SERVO_SPECS[SERVO_HEAD].min;
constexpr float TYPING_HEAD_HIGH =
  SERVO_SPECS[SERVO_HEAD].min + TYPING_HEAD_BAND_DEG;

constexpr float TYPING_BODY_MID =
  servoMid(SERVO_SPECS[SERVO_BODY]);
constexpr float TYPING_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);

constexpr float READING_HEAD_LOW =
  SERVO_SPECS[SERVO_HEAD].min;
constexpr float READING_HEAD_HIGH =
  SERVO_SPECS[SERVO_HEAD].min + READING_HEAD_BAND_DEG;
constexpr float READING_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);

}  // namespace anim
