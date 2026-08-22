#include <Arduino.h>
#include <math.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "animation/welcome.h"
#include "audio.h"
#include "servo_wrapper.h"
#include "servos.h"

using anim::easedLerp;
using anim::stopAnimServos;

namespace {

constexpr uint32_t RAISE_MS = 430;

uint32_t g_welcomeStartMs = 0;
uint32_t g_welcomeAudioStartMs = 0;
bool g_welcomeAudioStarted = false;

bool handRaiseComplete(uint32_t elapsed) {
  return elapsed >= RAISE_MS;
}

void parkWelcomeIdlePose() {
  anim::parkForTransition();
  servoAt(SERVO_HEAD).setTarget(
    anim::WELCOME_HEAD_MID,
    anim::TRANSITION_TORSO_SPEED_DEG_S
  );
}

void applyRaisePose(uint32_t now) {
  const float handAngle = easedLerp(
    anim::WELCOME_HAND_REST,
    anim::WELCOME_HAND_RAISED,
    g_welcomeStartMs,
    RAISE_MS,
    now
  );
  const float headAngle = easedLerp(
    anim::WELCOME_HEAD_MID,
    anim::WELCOME_HEAD_UP,
    g_welcomeStartMs,
    RAISE_MS,
    now
  );

  servoAt(SERVO_HAND_RIGHT).setPosition(handAngle);
  servoAt(SERVO_HEAD).setPosition(headAngle);
}

void applyAudioPose(uint32_t audioElapsed, uint32_t now) {
  float handAngle = anim::WELCOME_HAND_RAISED;
  float headAngle = anim::WELCOME_HEAD_UP;

  if (audioElapsed < WELCOME_AUDIO_QUESTION_END_MS) {
    handAngle = anim::WELCOME_HAND_RAISED;
    headAngle = anim::WELCOME_HEAD_UP;
  } else if (audioElapsed < WELCOME_AUDIO_END_MS) {
    handAngle = easedLerp(
      anim::WELCOME_HAND_RAISED,
      anim::WELCOME_HAND_REST,
      g_welcomeAudioStartMs + WELCOME_AUDIO_QUESTION_END_MS,
      WELCOME_AUDIO_END_MS - WELCOME_AUDIO_QUESTION_END_MS,
      now
    );
    headAngle = easedLerp(
      anim::WELCOME_HEAD_UP,
      anim::WELCOME_HEAD_MID,
      g_welcomeAudioStartMs + WELCOME_AUDIO_QUESTION_END_MS,
      WELCOME_AUDIO_END_MS - WELCOME_AUDIO_QUESTION_END_MS,
      now
    );
  } else {
    handAngle = anim::WELCOME_HAND_REST;
    headAngle = anim::WELCOME_HEAD_MID;
  }

  if (audioElapsed >= WELCOME_AUDIO_PAUSE_END_MS &&
      audioElapsed < WELCOME_AUDIO_QUESTION_END_MS) {
    const float questionElapsed =
      (float)(audioElapsed - WELCOME_AUDIO_PAUSE_END_MS);
    const float questionDuration =
      (float)(WELCOME_AUDIO_QUESTION_END_MS - WELCOME_AUDIO_PAUSE_END_MS);
    const float wiggle =
      sinf(
        questionElapsed / questionDuration * 2.0f * PI * 2.0f
      ) * anim::WELCOME_HAND_WIGGLE_DEG;
    handAngle = anim::WELCOME_HAND_RAISED + wiggle;

    const float nod =
      sinf(
        questionElapsed / questionDuration * 2.0f * PI
      ) * 3.0f;
    headAngle = anim::WELCOME_HEAD_UP + nod;
  }

  servoAt(SERVO_HAND_RIGHT).setPosition(handAngle);
  servoAt(SERVO_HEAD).setPosition(headAngle);
}

void updateWelcomeParkServos() {
  servoAt(SERVO_BODY).update();
  servoAt(SERVO_NECK).update();
  servoAt(SERVO_HAND_LEFT).update();
  servoAt(SERVO_HEAD).update();
}

void tryStartWelcomeAudio(uint32_t now, uint32_t elapsed) {
  if (g_welcomeAudioStarted) {
    return;
  }

  if (handRaiseComplete(elapsed)) {
    if (startWelcomePlayback()) {
      g_welcomeAudioStarted = true;
      g_welcomeAudioStartMs = now;
    }
  }
}

}  // namespace

bool welcomeAudioStarted() {
  return g_welcomeAudioStarted;
}

uint32_t welcomeAudioElapsed(uint32_t now) {
  if (!g_welcomeAudioStarted) {
    return 0;
  }

  return now - g_welcomeAudioStartMs;
}

void startWelcome() {
  stopAllWavPlayback();
  stopAnimServos();
  parkWelcomeIdlePose();

  g_welcomeStartMs = millis();
  g_welcomeAudioStartMs = 0;
  g_welcomeAudioStarted = false;
}

void updateWelcome(uint32_t now) {
  const uint32_t elapsed = now - g_welcomeStartMs;

  if (!g_welcomeAudioStarted) {
    applyRaisePose(now);
    updateWelcomeParkServos();
    tryStartWelcomeAudio(now, elapsed);
  } else {
    updateWelcomePlayback();
    applyAudioPose(welcomeAudioElapsed(now), now);
  }

  if (g_welcomeAudioStarted &&
      welcomeAudioElapsed(now) >= WELCOME_AUDIO_END_MS) {
    finishAnimation(now);
  } else if (!g_welcomeAudioStarted &&
      elapsed >= RAISE_MS + WELCOME_AUDIO_END_MS) {
    finishAnimation(now);
  }
}
