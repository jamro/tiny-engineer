#include <Arduino.h>

#include "animation.h"
#include "animation/abort.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "hardware/servo_wrapper.h"
#include "servos.h"

using anim::stopAnimServos;

namespace {

constexpr float ABORT_PREP_SPEED_DEG_S = 150.0f;
constexpr float ABORT_BEAT_SPEED_DEG_S = 58.0f;
constexpr float ABORT_RETURN_SPEED_DEG_S = 70.0f;
constexpr uint32_t ABORT_RETURN_TIMEOUT_MS = 1200;

enum class AbortPhase {
  PrepPose,
  PlayAudio,
  ReturnPose,
};

enum class AbortBeat {
  Fine,
  DidntWant,
  Finish,
  Anyway,
};

AbortPhase g_abortPhase = AbortPhase::PrepPose;
AbortBeat g_abortBeat = AbortBeat::Fine;
uint32_t g_abortAudioStartMs = 0;
uint32_t g_abortReturnStartMs = 0;
bool g_abortAudioStarted = false;

bool allAbortServosStopped() {
  return !servoAt(SERVO_BODY).isMoving()
    && !servoAt(SERVO_NECK).isMoving()
    && !servoAt(SERVO_HEAD).isMoving()
    && !servoAt(SERVO_HAND_LEFT).isMoving()
    && !servoAt(SERVO_HAND_RIGHT).isMoving();
}

void commandResignedPose(float speedDegS) {
  servoAt(SERVO_BODY).setTarget(
    anim::ABORT_BODY_DISMISSIVE,
    speedDegS
  );
  servoAt(SERVO_NECK).setTarget(
    anim::ABORT_NECK_SIDE,
    speedDegS
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ABORT_HEAD_UP,
    speedDegS
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::ABORT_HAND_LEFT_UP,
    speedDegS
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ABORT_HAND_RIGHT_UP,
    speedDegS
  );
}

void commandDidntWantBeat() {
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::ABORT_HAND_LEFT_SHRUG,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ABORT_HAND_RIGHT_SHRUG,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ABORT_HEAD_UP - 4.0f,
    ABORT_BEAT_SPEED_DEG_S
  );
}

void commandFinishBeat() {
  servoAt(SERVO_NECK).setTarget(
    anim::ABORT_NECK_OTHER_SIDE,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ABORT_HEAD_DIP,
    ABORT_BEAT_SPEED_DEG_S
  );
}

void commandAnywayBeat() {
  servoAt(SERVO_NECK).setTarget(
    anim::ABORT_NECK_SIDE,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    anim::ABORT_HEAD_UP + 2.0f,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_LEFT).setTarget(
    anim::ABORT_HAND_LEFT_UP,
    ABORT_BEAT_SPEED_DEG_S
  );
  servoAt(SERVO_HAND_RIGHT).setTarget(
    anim::ABORT_HAND_RIGHT_UP,
    ABORT_BEAT_SPEED_DEG_S
  );
}

void applyAudioBeat(uint32_t elapsed) {
  AbortBeat nextBeat = AbortBeat::Fine;

  if (elapsed >= ABORT_AUDIO_FINISH_END_MS) {
    nextBeat = AbortBeat::Anyway;
  } else if (elapsed >= ABORT_AUDIO_DIDNT_WANT_END_MS) {
    nextBeat = AbortBeat::Finish;
  } else if (elapsed >= ABORT_AUDIO_FINE_END_MS) {
    nextBeat = AbortBeat::DidntWant;
  }

  if (nextBeat == g_abortBeat) {
    return;
  }

  g_abortBeat = nextBeat;

  switch (g_abortBeat) {
    case AbortBeat::DidntWant:
      commandDidntWantBeat();
      break;
    case AbortBeat::Finish:
      commandFinishBeat();
      break;
    case AbortBeat::Anyway:
      commandAnywayBeat();
      break;
    case AbortBeat::Fine:
    default:
      commandResignedPose(ABORT_BEAT_SPEED_DEG_S);
      break;
  }
}

void beginReturnPose(uint32_t now) {
  g_abortPhase = AbortPhase::ReturnPose;
  g_abortReturnStartMs = now;

  anim::parkForTransition();
  servoAt(SERVO_HEAD).setTarget(
    servoMid(SERVO_SPECS[SERVO_HEAD]),
    ABORT_RETURN_SPEED_DEG_S
  );
}

}  // namespace

void startAbort() {
  stopAllWavPlayback();
  stopAnimServos();

  g_abortPhase = AbortPhase::PrepPose;
  g_abortBeat = AbortBeat::Fine;
  g_abortAudioStartMs = 0;
  g_abortReturnStartMs = 0;
  g_abortAudioStarted = false;

  commandResignedPose(ABORT_PREP_SPEED_DEG_S);
}

bool abortAudioStarted() {
  return g_abortAudioStarted;
}

uint32_t abortAudioElapsed(uint32_t now) {
  if (!g_abortAudioStarted) {
    return 0;
  }

  return now - g_abortAudioStartMs;
}

void updateAbort(uint32_t now) {
  switch (g_abortPhase) {
    case AbortPhase::PrepPose:
      updateAllServos();
      if (allAbortServosStopped()) {
        if (startAbortPlayback()) {
          g_abortAudioStarted = true;
          g_abortAudioStartMs = now;
          g_abortPhase = AbortPhase::PlayAudio;
        } else {
          beginReturnPose(now);
        }
      }
      break;

    case AbortPhase::PlayAudio: {
      updateAllServos();
      applyAudioBeat(abortAudioElapsed(now));
      const bool playing = updateAbortPlayback();
      if (!playing || abortAudioElapsed(now) >= ABORT_AUDIO_END_MS) {
        stopAbortPlayback();
        beginReturnPose(now);
      }
      break;
    }

    case AbortPhase::ReturnPose:
      updateAllServos();
      if (allAbortServosStopped() ||
          now - g_abortReturnStartMs >= ABORT_RETURN_TIMEOUT_MS) {
        finishAnimation(now);
      }
      break;
  }
}
