#include <Arduino.h>

#include "animation/util.h"
#include "servo_wrapper.h"
#include "servos.h"

namespace anim {

float randUnit() {
  return (float)(esp_random() & 0xFFFFu) / 65535.0f;
}

bool randChance(uint32_t percent) {
  return (esp_random() % 100u) < percent;
}

uint32_t randRangeMs(uint32_t lo, uint32_t hi) {
  if (hi <= lo) {
    return lo;
  }
  return lo + (esp_random() % (hi - lo + 1u));
}

float easeInOutCubic(float t) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }
  if (t < 0.5f) {
    return 4.0f * t * t * t;
  }
  const float f = -2.0f * t + 2.0f;
  return 1.0f - (f * f * f) / 2.0f;
}

void stopAnimServos() {
  servoAt(SERVO_HAND_LEFT).stop();
  servoAt(SERVO_HAND_RIGHT).stop();
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  servoAt(SERVO_BODY).stop();
}

void parkHandsAndBody() {
  servoAt(SERVO_BODY).setTarget(
    servoMid(SERVO_SPECS[SERVO_BODY])
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    SERVO_SPECS[SERVO_HAND_RIGHT].min
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    SERVO_SPECS[SERVO_HAND_LEFT].max
  );
}

void parkNonePose() {
  parkHandsAndBody();
  servoAt(SERVO_HEAD).setTarget(
    servoMid(SERVO_SPECS[SERVO_HEAD])
  );
  servoAt(SERVO_NECK).setTarget(
    servoMid(SERVO_SPECS[SERVO_NECK])
  );
}

void snapHeadToRangeHigh(float highDeg) {
  servoAt(SERVO_HEAD).setTarget(
    highDeg,
    SERVO_MAX_SPEED_DEG_S * 0.85f
  );
}

}  // namespace anim
