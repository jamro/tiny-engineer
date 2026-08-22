#include <Arduino.h>

#include "animation/constants.h"
#include "animation/error.h"
#include "animation/util.h"
#include "audio.h"
#include "servo_wrapper.h"
#include "servos.h"

using anim::randRangeMs;
using anim::stopAnimServos;

namespace {

constexpr float ERROR_PREP_SPEED_DEG_S = 135.0f;
constexpr float ERROR_HOLD_SPEED_DEG_S = 16.0f;

enum class ErrorPhase {
  ObstaclePose,
  PlayAudio,
  BlockedHold,
};

ErrorPhase g_errorPhase = ErrorPhase::ObstaclePose;
uint32_t g_errorAudioStartMs = 0;
bool g_errorAudioStarted = false;
uint32_t g_nextHoldMoveMs = 0;
bool g_holdShakeRight = false;

void commandObstaclePose() {
  servoAt(SERVO_BODY).setTarget(
    anim::ERROR_BODY_TASK_SIDE,
    ERROR_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ERROR_NECK_TASK_SIDE,
    ERROR_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ERROR_HEAD_CONCERNED,
    ERROR_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::ERROR_HAND_LEFT_TASK_POINT,
    ERROR_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ERROR_HAND_RIGHT_PRESENT,
    ERROR_PREP_SPEED_DEG_S
  );
}

bool allErrorServosStopped() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

void scheduleNextHoldMove(uint32_t now) {
  g_nextHoldMoveMs = now + randRangeMs(2200, 3800);
}

void commandHoldMove(uint32_t now) {
  if (now < g_nextHoldMoveMs) {
    return;
  }

  g_holdShakeRight = !g_holdShakeRight;
  const float sign = g_holdShakeRight ? 1.0f : -1.0f;

  servoAt(SERVO_HEAD).setTarget(
    anim::ERROR_HEAD_CONCERNED +
      sign * anim::ERROR_HOLD_HEAD_SHAKE_DEG,
    ERROR_HOLD_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ERROR_NECK_TASK_SIDE +
      sign * anim::ERROR_HOLD_NECK_SHAKE_DEG,
    ERROR_HOLD_SPEED_DEG_S
  );

  scheduleNextHoldMove(now);
}

void enterBlockedHold(uint32_t now) {
  g_errorPhase = ErrorPhase::BlockedHold;
  g_holdShakeRight = false;
  scheduleNextHoldMove(now);
}

}  // namespace

void startError() {
  stopAllWavPlayback();
  stopAnimServos();

  g_errorPhase = ErrorPhase::ObstaclePose;
  g_errorAudioStartMs = 0;
  g_errorAudioStarted = false;
  g_nextHoldMoveMs = 0;
  g_holdShakeRight = false;

  commandObstaclePose();
}

bool errorAudioStarted() {
  return g_errorAudioStarted;
}

uint32_t errorAudioElapsed(uint32_t now) {
  if (!g_errorAudioStarted) {
    return 0;
  }

  return now - g_errorAudioStartMs;
}

void updateError(uint32_t now) {
  switch (g_errorPhase) {
    case ErrorPhase::ObstaclePose:
      updateAllServos();
      if (allErrorServosStopped()) {
        if (startErrorPlayback()) {
          g_errorAudioStarted = true;
          g_errorAudioStartMs = now;
          g_errorPhase = ErrorPhase::PlayAudio;
        } else {
          enterBlockedHold(now);
        }
      }
      break;

    case ErrorPhase::PlayAudio:
      if (!updateErrorPlayback()) {
        enterBlockedHold(now);
      }
      break;

    case ErrorPhase::BlockedHold:
      updateAllServos();
      if (allErrorServosStopped()) {
        commandHoldMove(now);
      }
      break;
  }
}
