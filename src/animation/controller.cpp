#include <Arduino.h>
#include <string.h>

#include "animation.h"
#include "animation/abort.h"
#include "animation/attention.h"
#include "animation/error.h"
#include "animation/registry.h"
#include "animation/util.h"
#include "audio/audio.h"
#include "display/eyes.h"
#include "hardware/rgb.h"
#include "hardware/servo_wrapper.h"
#include "network/wifi_connect.h"
#include "settings.h"
#include "sleep.h"

namespace {

constexpr uint32_t MIN_ANIMATION_HOLD_MS = 1000;

AnimationId g_animation = AnimationId::None;
uint32_t g_animationStartedMs = 0;
uint32_t g_continuousStartedMs = 0;
bool g_hasPendingAnimation = false;
AnimationId g_pendingAnimation = AnimationId::None;

void applyAnimation(AnimationId id, uint32_t nowMs) {
  if (id != AnimationId::Attention) {
    stopAttentionPlayback();
  }
  if (id != AnimationId::Error) {
    stopErrorPlayback();
  }
  if (id != AnimationId::Abort) {
    stopAbortPlayback();
  }

  const ModeEntry* fromEntry = modeByAnimId(g_animation);
  const ModeEntry* entry = modeByAnimId(id);

  Serial.print("[anim] transition ");
  Serial.print(fromEntry->name);
  Serial.print(" -> ");
  Serial.println(entry->name);
  anim::logServoSnapshot("pre-transition");

  g_animation = id;
  g_animationStartedMs = nowMs;
  g_continuousStartedMs = nowMs;
  g_hasPendingAnimation = false;

  setEyeMode(entry->eyeMode, g_animationStartedMs);

  if (entry->startAnim != nullptr) {
    entry->startAnim(g_animationStartedMs);
  }

  setRgbForAnimation(id, nowMs);
  onAnimationApplied(id, nowMs);
}

bool animationHoldElapsed() {
  return (millis() - g_animationStartedMs) >= MIN_ANIMATION_HOLD_MS;
}

}  // namespace

void setAnimation(AnimationId id) {
  if (id == g_animation) {
    g_hasPendingAnimation = false;
    // Re-requesting the same continuous anim refreshes continuous_timeout
    // without restarting motion or the 1s min-hold clock.
    if (animationIsContinuous(id)) {
      g_continuousStartedMs = millis();
    }
    return;
  }

  if (animationHoldElapsed()) {
    applyAnimation(id, millis());
    return;
  }

  g_pendingAnimation = id;
  g_hasPendingAnimation = true;

  const uint32_t holdLeftMs =
    MIN_ANIMATION_HOLD_MS - (millis() - g_animationStartedMs);
  Serial.print("[anim] pending ");
  Serial.print(modeByAnimId(id)->name);
  Serial.print(" holdLeftMs=");
  Serial.println(holdLeftMs);
}

void finishAnimation(uint32_t nowMs) {
  applyAnimation(AnimationId::None, nowMs);
}

AnimationId getAnimation() {
  return g_animation;
}

bool hasPendingAnimation() {
  return g_hasPendingAnimation;
}

AnimationId pendingAnimation() {
  return g_pendingAnimation;
}

bool animationIsContinuous(AnimationId id) {
  return modeByAnimId(id)->continuous;
}

const char* animationName(AnimationId id) {
  return modeByAnimId(id)->name;
}

bool parseAnimationName(const char* name, AnimationId& out) {
  const ModeEntry* entry = modeByName(name);

  if (entry == nullptr) {
    return false;
  }

  out = entry->animId;
  return true;
}

void updateAnimation() {
  const uint32_t now = millis();

  if (g_hasPendingAnimation && animationHoldElapsed()) {
    applyAnimation(g_pendingAnimation, now);
  }

  if (animationIsContinuous(g_animation)) {
    const uint32_t timeoutMs =
      settingsContinuousTimeoutMin() * 60UL * 1000UL;
    if ((now - g_continuousStartedMs) >= timeoutMs) {
      setAnimation(AnimationId::Attention);
    }
  }

  const ModeEntry* entry = modeByAnimId(g_animation);

  if (entry->updateAnim != nullptr) {
    entry->updateAnim(now);
  } else {
    updateAllServos();
  }

  if (!wifiProvisioningMode()) {
    updateEyes(now);
  }
}
