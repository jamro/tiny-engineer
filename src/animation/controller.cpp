#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "animation/attention.h"
#include "animation/error.h"
#include "animation/reading.h"
#include "animation/ring.h"
#include "animation/thinking.h"
#include "animation/typing.h"
#include "animation/util.h"
#include "animation/welcome.h"
#include "audio.h"
#include "eyes.h"
#include "servo_wrapper.h"

namespace {

constexpr uint32_t MIN_ANIMATION_HOLD_MS = 1000;

AnimationId g_animation = AnimationId::None;
uint32_t g_animationStartedMs = 0;
bool g_hasPendingAnimation = false;
AnimationId g_pendingAnimation = AnimationId::None;

void startNone() {
  anim::stopAnimServos();
  anim::parkNonePose();
}

void applyAnimation(AnimationId id) {
  if (id != AnimationId::Attention) {
    stopAttentionPlayback();
  }
  if (id != AnimationId::Error) {
    stopErrorPlayback();
  }

  g_animation = id;
  g_animationStartedMs = millis();
  g_hasPendingAnimation = false;

  switch (id) {
    case AnimationId::Typing:
      setEyeMode(EyeMode::Typing, g_animationStartedMs);
      startTyping();
      break;
    case AnimationId::Reading:
      setEyeMode(EyeMode::Reading, g_animationStartedMs);
      startReading();
      break;
    case AnimationId::Thinking:
      setEyeMode(EyeMode::Thinking, g_animationStartedMs);
      startThinking(g_animationStartedMs);
      break;
    case AnimationId::Ring:
      setEyeMode(EyeMode::Ring, g_animationStartedMs);
      startRing();
      break;
    case AnimationId::Welcome:
      setEyeMode(EyeMode::Welcome, g_animationStartedMs);
      startWelcome();
      break;
    case AnimationId::Attention:
      setEyeMode(EyeMode::Attention, g_animationStartedMs);
      startAttention();
      break;
    case AnimationId::Error:
      setEyeMode(EyeMode::Error, g_animationStartedMs);
      startError();
      break;
    case AnimationId::None:
    default:
      setEyeMode(EyeMode::Idle, g_animationStartedMs);
      startNone();
      break;
  }
}

bool animationHoldElapsed() {
  return (millis() - g_animationStartedMs) >= MIN_ANIMATION_HOLD_MS;
}

}  // namespace

void setAnimation(AnimationId id) {
  if (id == g_animation) {
    g_hasPendingAnimation = false;
    return;
  }

  if (animationHoldElapsed()) {
    applyAnimation(id);
    return;
  }

  g_pendingAnimation = id;
  g_hasPendingAnimation = true;
}

void finishAnimation() {
  applyAnimation(AnimationId::None);
}

AnimationId getAnimation() {
  return g_animation;
}

const char* animationName(AnimationId id) {
  switch (id) {
    case AnimationId::Typing:
      return "typing";
    case AnimationId::Reading:
      return "reading";
    case AnimationId::Thinking:
      return "thinking";
    case AnimationId::Ring:
      return "ring";
    case AnimationId::Welcome:
      return "welcome";
    case AnimationId::Attention:
      return "attention";
    case AnimationId::Error:
      return "error";
    case AnimationId::None:
    default:
      return "none";
  }
}

bool parseAnimationName(const char* name, AnimationId& out) {
  if (name == nullptr) {
    return false;
  }

  if (strcmp(name, "none") == 0) {
    out = AnimationId::None;
    return true;
  }

  if (strcmp(name, "typing") == 0) {
    out = AnimationId::Typing;
    return true;
  }

  if (strcmp(name, "reading") == 0) {
    out = AnimationId::Reading;
    return true;
  }

  if (strcmp(name, "thinking") == 0) {
    out = AnimationId::Thinking;
    return true;
  }

  if (strcmp(name, "ring") == 0) {
    out = AnimationId::Ring;
    return true;
  }

  if (strcmp(name, "welcome") == 0) {
    out = AnimationId::Welcome;
    return true;
  }

  if (strcmp(name, "attention") == 0) {
    out = AnimationId::Attention;
    return true;
  }

  if (strcmp(name, "error") == 0) {
    out = AnimationId::Error;
    return true;
  }

  return false;
}

void updateAnimation() {
  if (g_hasPendingAnimation && animationHoldElapsed()) {
    applyAnimation(g_pendingAnimation);
  }

  const uint32_t now = millis();

  if (g_animation == AnimationId::None) {
    updateAllServos();
  } else {
    switch (g_animation) {
      case AnimationId::Typing:
        updateTyping(now);
        break;
      case AnimationId::Reading:
        updateReading(now);
        break;
      case AnimationId::Thinking:
        updateThinking(now);
        break;
      case AnimationId::Ring:
        updateRing(now);
        break;
      case AnimationId::Welcome:
        updateWelcome(now);
        break;
      case AnimationId::Attention:
        updateAttention(now);
        break;
      case AnimationId::Error:
        updateError(now);
        break;
      default:
        break;
    }
  }

  updateEyes(now);
}
