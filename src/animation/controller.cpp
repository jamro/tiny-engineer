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
#include "settings/settings.h"
#include "sleep.h"
#include "serial_log.h"

namespace {

constexpr uint32_t MIN_ANIMATION_HOLD_MS = 1000;

AnimationId g_animation = AnimationId::None;
uint32_t g_animationStartedMs = 0;
uint32_t g_continuousStartedMs = 0;
bool g_hasPendingAnimation = false;
AnimationId g_pendingAnimation = AnimationId::None;

// Awake/Opening wakeup: no clip; drop pending sleep; reset idle timer.
bool ignoreWakeupIfAwake(uint32_t nowMs) {
  if (sleepAllowsWakeup()) {
    return false;
  }

  if (g_hasPendingAnimation &&
      g_pendingAnimation == AnimationId::Sleep) {
    g_hasPendingAnimation = false;
  }

  noteActivity(nowMs);
  serialLogPrintln("[anim] ignore wakeup (awake)");
  return true;
}

void applyAnimation(AnimationId id, uint32_t nowMs) {
  if (id == AnimationId::Wakeup && ignoreWakeupIfAwake(nowMs)) {
    return;
  }

  // Only tear down a stream owned by the previous anim. Orphan Action/button
  // SFX must survive transitions into Sleep/Wakeup/typing/etc.
  if (animationDrivesWavStream(g_animation) && g_animation != id) {
    stopAllWavPlayback();
  }

  const ModeEntry* fromEntry = modeByAnimId(g_animation);
  const ModeEntry* entry = modeByAnimId(id);

  serialLogPrint("[anim] transition ");
  serialLogPrint(fromEntry->name);
  serialLogPrint(" -> ");
  serialLogPrintln(entry->name);
  anim::logServoSnapshot("pre-transition");

  if (g_animation == AnimationId::None && id != AnimationId::None) {
    ensureAllServoOutputs();
  }

  // Raise head out of sleep pose before the new anim claims servos.
  // None / Sleep / Wakeup must keep chin-down (or own the rise).
  if (id != AnimationId::None &&
      id != AnimationId::Sleep &&
      id != AnimationId::Wakeup) {
    prepareSleepWakePose();
  }

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

  if (id == AnimationId::Wakeup && ignoreWakeupIfAwake(millis())) {
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
  serialLogPrint("[anim] pending ");
  serialLogPrint(modeByAnimId(id)->name);
  serialLogPrint(" holdLeftMs=");
  serialLogPrintln(holdLeftMs);
}

void setAnimationImmediately(AnimationId id) {
  applyAnimation(id, millis());
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

bool animationDrivesWavStream(AnimationId id) {
  switch (id) {
    case AnimationId::Ring:
    case AnimationId::Welcome:
    case AnimationId::Attention:
    case AnimationId::Error:
    case AnimationId::Abort:
    case AnimationId::Dead:
      return true;
    default:
      return false;
  }
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
