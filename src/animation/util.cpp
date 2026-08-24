#include <Arduino.h>

#include "animation/constants.h"
#include "animation/util.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"
#include "serial_log.h"

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

float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

void beginEasedMove(
  EasedMove& move,
  float from,
  float to,
  uint32_t startMs,
  uint32_t durationMs
) {
  move.from = from;
  move.to = to;
  move.startMs = startMs;
  move.durationMs = durationMs > 0 ? durationMs : 1;
  move.active = true;
}

float easedMoveValue(const EasedMove& move, uint32_t now) {
  if (!move.active) {
    return move.to;
  }
  if (now < move.startMs) {
    return move.from;
  }
  const uint32_t elapsed = now - move.startMs;
  if (elapsed >= move.durationMs) {
    return move.to;
  }
  const float t = (float)elapsed / (float)move.durationMs;
  return lerp(move.from, move.to, easeInOutCubic(t));
}

bool easedMoveDone(const EasedMove& move, uint32_t now) {
  if (!move.active) {
    return true;
  }
  return now >= move.startMs + move.durationMs;
}

float easedLerp(
  float from,
  float to,
  uint32_t startMs,
  uint32_t durationMs,
  uint32_t now
) {
  if (durationMs == 0) {
    return to;
  }
  if (now <= startMs) {
    return from;
  }
  const uint32_t elapsed = now - startMs;
  if (elapsed >= durationMs) {
    return to;
  }
  const float t = (float)elapsed / (float)durationMs;
  return lerp(from, to, easeInOutCubic(t));
}

namespace {

void logServoAxis(const char* name, int index) {
  const ServoWrapper& servo = servoAt(index);
  serialLogPrint("  ");
  serialLogPrint(name);
  serialLogPrint('=');
  serialLogPrint(servo.angle(), 1);
  serialLogPrintln(servo.isMoving() ? " (moving)" : " (still)");
}

}  // namespace

void logServoSnapshot(const char* tag) {
  serialLogPrint("[anim] ");
  serialLogPrintln(tag);
  logServoAxis("head", SERVO_HEAD);
  logServoAxis("neck", SERVO_NECK);
  logServoAxis("handL", SERVO_HAND_LEFT);
  logServoAxis("handR", SERVO_HAND_RIGHT);
  logServoAxis("body", SERVO_BODY);
}

void stopAnimServos() {
  servoAt(SERVO_HAND_LEFT).stop();
  servoAt(SERVO_HAND_RIGHT).stop();
  servoAt(SERVO_HEAD).stop();
  servoAt(SERVO_NECK).stop();
  servoAt(SERVO_BODY).stop();
}

void parkTorso(float speedDegS) {
  const float bodyMid = servoMid(SERVO_SPECS[SERVO_BODY]);
  const float neckMid = servoMid(SERVO_SPECS[SERVO_NECK]);

  serialLogPrint("[anim] parkTorso body ");
  serialLogPrint(servoAt(SERVO_BODY).angle(), 1);
  serialLogPrint(" -> ");
  serialLogPrint(bodyMid, 1);
  serialLogPrint(" neck ");
  serialLogPrint(servoAt(SERVO_NECK).angle(), 1);
  serialLogPrint(" -> ");
  serialLogPrint(neckMid, 1);
  serialLogPrint(" speed=");
  serialLogPrintln(speedDegS, 1);

  servoAt(SERVO_BODY).setTarget(bodyMid, speedDegS);
  servoAt(SERVO_NECK).setTarget(neckMid, speedDegS);
}

void parkHands(float speedDegS) {
  const float handRightRest = SERVO_SPECS[SERVO_HAND_RIGHT].min;
  const float handLeftRest = SERVO_SPECS[SERVO_HAND_LEFT].max;

  servoAt(SERVO_HAND_RIGHT).setTarget(handRightRest, speedDegS);
  servoAt(SERVO_HAND_LEFT).setTarget(handLeftRest, speedDegS);
}

void parkForTransition() {
  parkTorso(TRANSITION_TORSO_SPEED_DEG_S);
  parkHands(TRANSITION_HAND_SPEED_DEG_S);
}

bool isTransitionParkComplete() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

void parkHandsAndBody() {
  parkForTransition();
}

void parkNonePose() {
  parkForTransition();
  servoAt(SERVO_HEAD).setTarget(
    servoMid(SERVO_SPECS[SERVO_HEAD]),
    TRANSITION_TORSO_SPEED_DEG_S
  );
}

void snapHeadToRangeHigh(float highDeg) {
  servoAt(SERVO_HEAD).setTarget(
    highDeg,
    SERVO_MAX_SPEED_DEG_S * 0.85f
  );
}

}  // namespace anim
