#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "animation/reading.h"
#include "animation/ring.h"
#include "animation/thinking.h"
#include "animation/typing.h"
#include "animation/util.h"
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
  g_animation = id;
  g_animationStartedMs = millis();
  g_hasPendingAnimation = false;

  switch (id) {
    case AnimationId::Typing:
      startTyping();
      break;
    case AnimationId::Reading:
      startReading();
      break;
    case AnimationId::Thinking:
      startThinking(g_animationStartedMs);
      break;
    case AnimationId::Ring:
      startRing();
      break;
    case AnimationId::None:
    default:
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
      default:
        break;
    }
  }

  updateEyes(now);
}
