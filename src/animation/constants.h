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

constexpr float RING_HEAD_START =
  servoMid(SERVO_SPECS[SERVO_HEAD]) + 10.0f;
constexpr float RING_BODY_START = SERVO_SPECS[SERVO_BODY].min;
constexpr float RING_RIGHT_START = SERVO_SPECS[SERVO_HAND_RIGHT].max;
constexpr float RING_LEFT_START =
  SERVO_SPECS[SERVO_HAND_LEFT].max - 10.0f;
constexpr float RING_NECK_START =
  servoMid(SERVO_SPECS[SERVO_NECK]);
constexpr float RING_RIGHT_STRIKE =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + 5.0f;
constexpr float RING_RIGHT_BOUNCE =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + 20.0f;
constexpr float RING_HEAD_STRIKE = SERVO_SPECS[SERVO_HEAD].min;

constexpr float WELCOME_HEAD_UP =
  servoMid(SERVO_SPECS[SERVO_HEAD]) + 10.0f;
constexpr float WELCOME_HEAD_MID =
  servoMid(SERVO_SPECS[SERVO_HEAD]);
constexpr float WELCOME_HAND_REST =
  SERVO_SPECS[SERVO_HAND_RIGHT].min;
constexpr float WELCOME_HAND_RAISED =
  SERVO_SPECS[SERVO_HAND_RIGHT].max;
constexpr float WELCOME_HAND_WIGGLE_DEG = 8.0f;

constexpr float ATTENTION_BODY_MID =
  servoMid(SERVO_SPECS[SERVO_BODY]);
constexpr float ATTENTION_NECK_MID =
  servoMid(SERVO_SPECS[SERVO_NECK]);
constexpr float ATTENTION_HEAD_READY =
  servoMid(SERVO_SPECS[SERVO_HEAD]) + 6.0f;
constexpr float ATTENTION_HAND_LEFT_PARKED =
  SERVO_SPECS[SERVO_HAND_LEFT].max;
constexpr float ATTENTION_HAND_RIGHT_REST =
  SERVO_SPECS[SERVO_HAND_RIGHT].min;
constexpr float ATTENTION_HAND_RIGHT_RAISED =
  SERVO_SPECS[SERVO_HAND_RIGHT].min + 48.0f;
constexpr float ATTENTION_WAIT_HEAD_NOD_DEG = 3.0f;
constexpr float ATTENTION_WAIT_NECK_TILT_DEG = 4.0f;

}  // namespace anim
