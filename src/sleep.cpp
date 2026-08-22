#include <Arduino.h>

#include "animation.h"
#include "display/eyes.h"
#include "display/oled.h"
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
uint32_t g_lastApiActivityMs = 0;

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
  g_lastApiActivityMs = millis();
}

void touchApiActivity() {
  const uint32_t now = millis();
  g_lastApiActivityMs = now;

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
  switch (g_state) {
    case SleepState::Awake:
      if (getAnimation() == AnimationId::None &&
          now - g_lastApiActivityMs >= settingsSleepTimeoutMs()) {
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
        g_lastApiActivityMs = now;
      }
      break;
  }
}

bool isSleeping() {
  return g_state == SleepState::Sleeping;
}
