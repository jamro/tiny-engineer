#include <Arduino.h>

#include "animation.h"
#include "display/eyes.h"
#include "display/oled.h"
#include "network/wifi_connect.h"
#include "settings.h"
#include "sleep.h"

namespace {

enum class SleepState {
  Awake,
  Closing,
  Sleeping,
  Opening
};

SleepState g_state = SleepState::Awake;
uint32_t g_idleSinceMs = 0;

void wakeFromSleep(uint32_t now) {
  wakeOled();
  startEyesForWake(now);
  g_state = SleepState::Opening;
}

void beginSleepClosing(uint32_t now) {
  requestSleepEyeClose(now);
  g_state = SleepState::Closing;
}

}  // namespace

void initSleep() {
  g_idleSinceMs = millis();
}

void onAnimationApplied(AnimationId id, uint32_t now) {
  if (id == AnimationId::None) {
    g_idleSinceMs = now;
    return;
  }

  switch (g_state) {
    case SleepState::Sleeping:
      wakeFromSleep(now);
      break;
    case SleepState::Closing:
      requestSleepEyeOpen(now);
      g_state = SleepState::Opening;
      break;
    default:
      break;
  }
}

void updateSleep(uint32_t now) {
  if (wifiProvisioningMode()) {
    return;
  }

  switch (g_state) {
    case SleepState::Awake:
      // g_idleSinceMs may be set with millis() after this loop's `now`
      // (e.g. POST /anim during pollHttpServer). Guard against underflow.
      if (getAnimation() == AnimationId::None &&
          now >= g_idleSinceMs &&
          now - g_idleSinceMs >= settingsSleepTimeoutMs()) {
        beginSleepClosing(now);
      }
      break;

    case SleepState::Closing:
      if (updateSleepEyes(now) == SleepEyeResult::CloseComplete) {
        blankOled();
        sleepOled();
        stopEyes();
        g_state = SleepState::Sleeping;
      }
      break;

    case SleepState::Sleeping:
      break;

    case SleepState::Opening:
      if (updateSleepEyes(now) == SleepEyeResult::OpenComplete) {
        g_state = SleepState::Awake;
        if (getAnimation() == AnimationId::None) {
          g_idleSinceMs = now;
        }
      }
      break;
  }
}

bool isSleeping() {
  return g_state == SleepState::Sleeping;
}
