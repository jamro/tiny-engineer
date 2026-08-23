#include <Arduino.h>

#include "animation.h"
#include "animation/attention.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::easedLerp;
using anim::randChance;
using anim::randRangeMs;
using anim::stopAnimServos;

namespace {

constexpr float ATTENTION_PREP_SPEED_DEG_S = 140.0f;
constexpr float ATTENTION_WAIT_SPEED_DEG_S = 18.0f;
constexpr float ATTENTION_WAVE_SPEED_DEG_S = 55.0f;

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
uint32_t g_nextHandWaveMs = 0;
bool g_waitNodHigh = false;
bool g_waitNeckRight = false;
bool g_handWaveHigh = false;
uint8_t g_handWaveBurstLeft = 0;

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

void scheduleNextHandWave(uint32_t now) {
  g_nextHandWaveMs = now + randRangeMs(2400, 5200);
  g_handWaveBurstLeft = 0;
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

void commandHandWave(uint32_t now) {
  if (now < g_nextHandWaveMs) {
    return;
  }

  if (g_handWaveBurstLeft == 0) {
    // Skip some scheduled slots so waves feel irregular.
    if (!randChance(70)) {
      scheduleNextHandWave(now);
      return;
    }
    g_handWaveBurstLeft = (uint8_t)(randChance(40) ? 3 : 2);
    g_handWaveHigh = true;
  }

  const float handOffset = g_handWaveHigh
    ? anim::ATTENTION_WAIT_HAND_WAVE_DEG
    : -anim::ATTENTION_WAIT_HAND_WAVE_DEG * 0.45f;

  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ATTENTION_HAND_RIGHT_RAISED + handOffset,
    ATTENTION_WAVE_SPEED_DEG_S
  );

  g_handWaveHigh = !g_handWaveHigh;
  g_handWaveBurstLeft--;

  if (g_handWaveBurstLeft == 0) {
    scheduleNextHandWave(now);
  } else {
    g_nextHandWaveMs = now + randRangeMs(220, 380);
  }
}

void applyAttentionAudioPose(uint32_t audioElapsed, uint32_t now) {
  float neckAngle = anim::ATTENTION_NECK_MID;
  float headAngle = anim::ATTENTION_HEAD_READY;
  float handAngle = anim::ATTENTION_HAND_RIGHT_RAISED;

  if (audioElapsed < ATTENTION_AUDIO_PST_END_MS) {
    neckAngle = anim::ATTENTION_NECK_MID;
    headAngle = anim::ATTENTION_HEAD_READY;
    handAngle = anim::ATTENTION_HAND_RIGHT_RAISED;
  } else if (audioElapsed < ATTENTION_AUDIO_HUMAN_END_MS) {
    neckAngle = easedLerp(
      anim::ATTENTION_NECK_MID,
      anim::ATTENTION_NECK_MID + anim::ATTENTION_HUMAN_NECK_GLANCE_DEG,
      g_attentionAudioStartMs + ATTENTION_AUDIO_PST_END_MS,
      ATTENTION_AUDIO_HUMAN_END_MS - ATTENTION_AUDIO_PST_END_MS,
      now
    );
    headAngle = easedLerp(
      anim::ATTENTION_HEAD_READY,
      anim::ATTENTION_HEAD_READY + anim::ATTENTION_HUMAN_HEAD_NOD_DEG,
      g_attentionAudioStartMs + ATTENTION_AUDIO_PST_END_MS,
      ATTENTION_AUDIO_HUMAN_END_MS - ATTENTION_AUDIO_PST_END_MS,
      now
    );
    handAngle = anim::ATTENTION_HAND_RIGHT_RAISED;
  } else if (audioElapsed < ATTENTION_AUDIO_END_MS) {
    neckAngle = easedLerp(
      anim::ATTENTION_NECK_MID + anim::ATTENTION_HUMAN_NECK_GLANCE_DEG,
      anim::ATTENTION_NECK_MID - anim::ATTENTION_LOOK_NECK_GLANCE_DEG,
      g_attentionAudioStartMs + ATTENTION_AUDIO_HUMAN_END_MS,
      ATTENTION_AUDIO_END_MS - ATTENTION_AUDIO_HUMAN_END_MS,
      now
    );
    headAngle = easedLerp(
      anim::ATTENTION_HEAD_READY + anim::ATTENTION_HUMAN_HEAD_NOD_DEG,
      anim::ATTENTION_HEAD_READY - anim::ATTENTION_LOOK_HEAD_NOD_DEG,
      g_attentionAudioStartMs + ATTENTION_AUDIO_HUMAN_END_MS,
      ATTENTION_AUDIO_END_MS - ATTENTION_AUDIO_HUMAN_END_MS,
      now
    );
    handAngle = easedLerp(
      anim::ATTENTION_HAND_RIGHT_RAISED,
      anim::ATTENTION_HAND_POINT,
      g_attentionAudioStartMs + ATTENTION_AUDIO_HUMAN_END_MS,
      ATTENTION_AUDIO_END_MS - ATTENTION_AUDIO_HUMAN_END_MS,
      now
    );
  } else {
    neckAngle = anim::ATTENTION_NECK_MID - anim::ATTENTION_LOOK_NECK_GLANCE_DEG;
    headAngle = anim::ATTENTION_HEAD_READY - anim::ATTENTION_LOOK_HEAD_NOD_DEG;
    handAngle = anim::ATTENTION_HAND_POINT;
  }

  servoAt(SERVO_NECK).setPosition(neckAngle);
  servoAt(SERVO_HEAD).setPosition(headAngle);
  servoAt(SERVO_HAND_RIGHT).setPosition(handAngle);
}

void enterAwaitInput(uint32_t now) {
  g_attentionPhase = AttentionPhase::AwaitInput;
  g_awaitInputStartedMs = now;
  g_waitNodHigh = false;
  g_waitNeckRight = false;
  g_handWaveHigh = false;
  g_handWaveBurstLeft = 0;
  scheduleNextWaitMove(now);
  scheduleNextHandWave(now);
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ATTENTION_HAND_RIGHT_RAISED,
    ATTENTION_WAVE_SPEED_DEG_S
  );
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
  g_nextHandWaveMs = 0;
  g_waitNodHigh = false;
  g_waitNeckRight = false;
  g_handWaveHigh = false;
  g_handWaveBurstLeft = 0;

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
      applyAttentionAudioPose(attentionAudioElapsed(now), now);
      updateAllServos();
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
      if (!servoAt(SERVO_HEAD).isMoving() && !servoAt(SERVO_NECK).isMoving()) {
        commandWaitMove(now);
      }
      if (!servoAt(SERVO_HAND_RIGHT).isMoving()) {
        commandHandWave(now);
      }
      break;
  }
}
