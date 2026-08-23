#include <Arduino.h>

#include "animation.h"
#include "animation/attention.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::randChance;
using anim::randRangeMs;
using anim::stopAnimServos;

namespace {

constexpr float ATTENTION_PREP_SPEED_DEG_S = 140.0f;
constexpr float ATTENTION_WAIT_SPEED_DEG_S = 18.0f;

enum class AttentionPhase {
  PrepPose,
  PlayAudio,
  AwaitInput,
};

AttentionPhase g_attentionPhase = AttentionPhase::PrepPose;
uint32_t g_attentionAudioStartMs = 0;
bool g_attentionAudioStarted = false;
uint32_t g_awaitInputStartedMs = 0;
uint32_t g_nextWaitMoveMs = 0;
bool g_waitNodHigh = false;
bool g_waitNeckRight = false;

void commandPrepPose() {
  servoAt(SERVO_BODY).setTarget(
    anim::ATTENTION_BODY_MID,
    ATTENTION_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ATTENTION_NECK_MID,
    ATTENTION_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ATTENTION_HEAD_READY,
    ATTENTION_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::ATTENTION_HAND_LEFT_PARKED,
    ATTENTION_PREP_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ATTENTION_HAND_RIGHT_RAISED,
    ATTENTION_PREP_SPEED_DEG_S
  );
}

bool allAttentionServosStopped() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

void scheduleNextWaitMove(uint32_t now) {
  g_nextWaitMoveMs = now + randRangeMs(1800, 3200);
}

void commandWaitMove(uint32_t now) {
  if (now < g_nextWaitMoveMs) {
    return;
  }

  g_waitNodHigh = !g_waitNodHigh;

  if (randChance(45)) {
    g_waitNeckRight = !g_waitNeckRight;
  }

  const float headOffset = g_waitNodHigh
    ? anim::ATTENTION_WAIT_HEAD_NOD_DEG
    : -anim::ATTENTION_WAIT_HEAD_NOD_DEG;
  const float neckOffset = g_waitNeckRight
    ? anim::ATTENTION_WAIT_NECK_TILT_DEG
    : -anim::ATTENTION_WAIT_NECK_TILT_DEG;

  servoAt(SERVO_HEAD).setTarget(
    anim::ATTENTION_HEAD_READY + headOffset,
    ATTENTION_WAIT_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ATTENTION_NECK_MID + neckOffset,
    ATTENTION_WAIT_SPEED_DEG_S
  );

  scheduleNextWaitMove(now);
}

void enterAwaitInput(uint32_t now) {
  g_attentionPhase = AttentionPhase::AwaitInput;
  g_awaitInputStartedMs = now;
  g_waitNodHigh = false;
  g_waitNeckRight = false;
  scheduleNextWaitMove(now);
}

}  // namespace

void startAttention() {
  stopAllWavPlayback();
  stopAnimServos();

  g_attentionPhase = AttentionPhase::PrepPose;
  g_attentionAudioStartMs = 0;
  g_attentionAudioStarted = false;
  g_awaitInputStartedMs = 0;
  g_nextWaitMoveMs = 0;
  g_waitNodHigh = false;
  g_waitNeckRight = false;

  commandPrepPose();
}

bool attentionAudioStarted() {
  return g_attentionAudioStarted;
}

uint32_t attentionAudioElapsed(uint32_t now) {
  if (!g_attentionAudioStarted) {
    return 0;
  }

  return now - g_attentionAudioStartMs;
}

void updateAttention(uint32_t now) {
  switch (g_attentionPhase) {
    case AttentionPhase::PrepPose:
      updateAllServos();
      if (allAttentionServosStopped()) {
        if (startAttentionPlayback()) {
          g_attentionAudioStarted = true;
          g_attentionAudioStartMs = now;
          g_attentionPhase = AttentionPhase::PlayAudio;
        } else {
          enterAwaitInput(now);
        }
      }
      break;

    case AttentionPhase::PlayAudio:
      if (!updateAttentionPlayback()) {
        enterAwaitInput(now);
      }
      break;

    case AttentionPhase::AwaitInput:
      if ((now - g_awaitInputStartedMs) >= anim::NON_CONTINUOUS_HOLD_MS) {
        finishAnimation(now);
        break;
      }
      updateAllServos();
      if (allAttentionServosStopped()) {
        commandWaitMove(now);
      }
      break;
  }
}
