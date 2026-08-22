#include <Arduino.h>
#include <math.h>

#include "animation.h"
#include "animation/constants.h"
#include "animation/util.h"
#include "animation/welcome.h"
#include "audio.h"
#include "servo_wrapper.h"
#include "servos.h"

using anim::stopAnimServos;

namespace {

constexpr float WELCOME_MOVE_SPEED_DEG_S = SERVO_MAX_SPEED_DEG_S;
constexpr uint32_t RAISE_MS = 430;

uint32_t g_welcomeStartMs = 0;
uint32_t g_welcomeAudioStartMs = 0;
bool g_welcomeAudioStarted = false;

float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

bool handRaiseComplete() {
  return !servoAt(SERVO_HAND_RIGHT).isMoving()
    && servoAt(SERVO_HAND_RIGHT).angle() >=
      anim::WELCOME_HAND_RAISED - 2.0f;
}

void parkWelcomeIdlePose() {
  anim::parkHandsAndBody();
  servoAt(SERVO_HEAD).setTarget(
    anim::WELCOME_HEAD_MID,
    WELCOME_MOVE_SPEED_DEG_S
  );
  servoAt(SERVO_NECK).setTarget(
    servoMid(SERVO_SPECS[SERVO_NECK]),
    WELCOME_MOVE_SPEED_DEG_S
  );
}

void applyRaisePose(uint32_t elapsed) {
  const float t = anim::easeInOutCubic(
    constrain(
      (float)elapsed / (float)RAISE_MS,
      0.0f,
      1.0f
    )
  );

  servoAt(SERVO_HAND_RIGHT).setTarget(
    lerp(
      anim::WELCOME_HAND_REST,
      anim::WELCOME_HAND_RAISED,
      t
    ),
    WELCOME_MOVE_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    lerp(
      anim::WELCOME_HEAD_MID,
      anim::WELCOME_HEAD_UP,
      t
    ),
    WELCOME_MOVE_SPEED_DEG_S
  );
}

void applyAudioPose(uint32_t audioElapsed) {
  float handAngle = anim::WELCOME_HAND_RAISED;
  float headAngle = anim::WELCOME_HEAD_UP;

  if (audioElapsed < WELCOME_AUDIO_LOGIN_END_MS) {
    handAngle = anim::WELCOME_HAND_RAISED;
    headAngle = anim::WELCOME_HEAD_UP;
  } else if (audioElapsed < WELCOME_AUDIO_ACCEPTED_END_MS) {
    const float t = anim::easeInOutCubic(
      (float)(audioElapsed - WELCOME_AUDIO_LOGIN_END_MS) /
      (float)(WELCOME_AUDIO_ACCEPTED_END_MS - WELCOME_AUDIO_LOGIN_END_MS)
    );
    handAngle = lerp(
      anim::WELCOME_HAND_RAISED,
      anim::WELCOME_HAND_REST,
      t
    );
    headAngle = lerp(
      anim::WELCOME_HEAD_UP,
      anim::WELCOME_HEAD_MID,
      t
    );
  } else {
    handAngle = anim::WELCOME_HAND_REST;
    headAngle = anim::WELCOME_HEAD_MID;
  }

  if (audioElapsed >= WELCOME_AUDIO_PAUSE_END_MS &&
      audioElapsed < WELCOME_AUDIO_LOGIN_END_MS) {
    const float loginElapsed =
      (float)(audioElapsed - WELCOME_AUDIO_PAUSE_END_MS);
    const float loginDuration =
      (float)(WELCOME_AUDIO_LOGIN_END_MS - WELCOME_AUDIO_PAUSE_END_MS);
    const float wiggle =
      sinf(
        loginElapsed / loginDuration * 2.0f * PI * 2.0f
      ) * anim::WELCOME_HAND_WIGGLE_DEG;
    handAngle = anim::WELCOME_HAND_RAISED + wiggle;

    const float nod =
      sinf(
        loginElapsed / loginDuration * 2.0f * PI
      ) * 3.0f;
    headAngle = anim::WELCOME_HEAD_UP + nod;
  }

  servoAt(SERVO_HAND_RIGHT).setTarget(
    handAngle,
    WELCOME_MOVE_SPEED_DEG_S
  );
  servoAt(SERVO_HEAD).setTarget(
    headAngle,
    WELCOME_MOVE_SPEED_DEG_S
  );
}

void tryStartWelcomeAudio(uint32_t now, uint32_t elapsed) {
  if (g_welcomeAudioStarted) {
    return;
  }

  if (handRaiseComplete() || elapsed >= RAISE_MS) {
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
  stopBellPlayback();
  stopWelcomePlayback();
  stopAnimServos();
  parkWelcomeIdlePose();

  g_welcomeStartMs = millis();
  g_welcomeAudioStartMs = 0;
  g_welcomeAudioStarted = false;
}

void updateWelcome(uint32_t now) {
  updateAllServos();

  const uint32_t elapsed = now - g_welcomeStartMs;

  if (!g_welcomeAudioStarted) {
    applyRaisePose(elapsed);
    tryStartWelcomeAudio(now, elapsed);
  } else {
    updateWelcomePlayback();
    applyAudioPose(welcomeAudioElapsed(now));
  }

  if (g_welcomeAudioStarted &&
      welcomeAudioElapsed(now) >= WELCOME_AUDIO_ACCEPTED_END_MS) {
    finishAnimation();
  } else if (!g_welcomeAudioStarted &&
      elapsed >= RAISE_MS + WELCOME_AUDIO_ACCEPTED_END_MS) {
    finishAnimation();
  }
}
