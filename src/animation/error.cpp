#include <Arduino.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/error.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::randRangeMs;
using anim::stopAnimServos;

namespace {

constexpr float ERROR_PREP_SPEED_DEG_S = 135.0f;
constexpr float ERROR_GLANCE_SPEED_DEG_S = 42.0f;

enum class ErrorLook {
  Task,
  Human,
  Away,
};

enum class ErrorPhase {
  ObstaclePose,
  PlayAudio,
  BlockedHold,
};

ErrorPhase g_errorPhase = ErrorPhase::ObstaclePose;
uint32_t g_errorAudioStartMs = 0;
bool g_errorAudioStarted = false;
uint32_t g_blockedHoldStartedMs = 0;
uint32_t g_nextHoldMoveMs = 0;
bool g_holdNodLow = false;
ErrorLook g_currentLook = ErrorLook::Task;

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
  g_nextHoldMoveMs = now + randRangeMs(450, 950);
}

ErrorLook nextNervousLook() {
  switch (g_currentLook) {
    case ErrorLook::Task:
      return ErrorLook::Human;
    case ErrorLook::Human:
      return ErrorLook::Away;
    case ErrorLook::Away:
    default:
      return ErrorLook::Task;
  }
}

void commandNervousLook(uint32_t now) {
  if (now < g_nextHoldMoveMs) {
    return;
  }

  g_currentLook = nextNervousLook();
  g_holdNodLow = !g_holdNodLow;

  float neckOffset = 0.0f;
  float headOffset = g_holdNodLow
    ? -anim::ERROR_WORRY_HEAD_NOD_DEG
    : anim::ERROR_WORRY_HEAD_NOD_DEG;

  switch (g_currentLook) {
    case ErrorLook::Task:
      neckOffset = 0.0f;
      headOffset -= anim::ERROR_HELP_HEAD_GLANCE_DEG;
      break;
    case ErrorLook::Human:
      neckOffset = anim::ERROR_HELP_NECK_GLANCE_DEG;
      headOffset += anim::ERROR_HELP_HEAD_GLANCE_DEG;
      break;
    case ErrorLook::Away:
      neckOffset = -anim::ERROR_AWAY_NECK_GLANCE_DEG;
      break;
  }

  servoAt(SERVO_HEAD).setTarget(
    anim::ERROR_HEAD_CONCERNED + headOffset,
    ERROR_GLANCE_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ERROR_NECK_TASK_SIDE + neckOffset,
    ERROR_GLANCE_SPEED_DEG_S
  );

  scheduleNextHoldMove(now);
}

void enterBlockedHold(uint32_t now) {
  g_errorPhase = ErrorPhase::BlockedHold;
  g_blockedHoldStartedMs = now;
  g_holdNodLow = false;
  g_currentLook = ErrorLook::Task;
  scheduleNextHoldMove(now);
}

}  // namespace

void startError() {
  stopAllWavPlayback();
  stopAnimServos();

  g_errorPhase = ErrorPhase::ObstaclePose;
  g_errorAudioStartMs = 0;
  g_errorAudioStarted = false;
  g_blockedHoldStartedMs = 0;
  g_nextHoldMoveMs = 0;
  g_holdNodLow = false;
  g_currentLook = ErrorLook::Task;

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
          g_nextHoldMoveMs = now + ERROR_AUDIO_UHOH_END_MS;
          g_errorPhase = ErrorPhase::PlayAudio;
        } else {
          enterBlockedHold(now);
        }
      }
      break;

    case ErrorPhase::PlayAudio:
      updateAllServos();
      if (allErrorServosStopped()) {
        commandNervousLook(now);
      }
      if (!updateErrorPlayback()) {
        enterBlockedHold(now);
      }
      break;

    case ErrorPhase::BlockedHold:
      if ((now - g_blockedHoldStartedMs) >= anim::NON_CONTINUOUS_HOLD_MS) {
        finishAnimation(now);
        break;
      }
      updateAllServos();
      if (allErrorServosStopped()) {
        commandNervousLook(now);
      }
      break;
  }
}
