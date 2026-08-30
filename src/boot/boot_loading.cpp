#include "boot/boot_loading.h"

#include <Arduino.h>
#include <cstring>

#include "animation/constants.h"
#include "animation/wakeup.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/oled.h"
#include "hardware/servo_wrapper.h"
#include "pins.h"
#include "servos.h"
#include "settings.h"

namespace {

constexpr uint32_t kFrameMs = 16;

float neckMid() {
  return servoMid(SERVO_SPECS[SERVO_NECK]);
}

}  // namespace

bool bootLoadingIsProgress() {
  return strcmp(settingsLoading(), "progress") == 0;
}

void bootShowProgress(
  int step,
  int totalSteps,
  const char* label
) {
  if (!bootLoadingIsProgress()) {
    return;
  }

  showBootProgress(step, totalSteps, label);
}

void bootBeginSleepingFace() {
  startEyes();
  blinkSetOpenAmount(eyes::BLINK_CLOSED_AMOUNT);
  blinkBeginIdle(millis());
  blinkSetNextBlinkMs(millis() + 60000UL);
  eyes::requestForceRedraw();
  updateEyes(millis());
}

bool bootSleepInertiaUsesServos() {
  return !bootLoadingIsProgress() && settingsWelcomeEnabled();
}

void bootSnapSleepPose() {
  const float mid = neckMid();
  const float handRightDown = SERVO_SPECS[SERVO_HAND_RIGHT].min;
  const float handLeftDown = SERVO_SPECS[SERVO_HAND_LEFT].max;

  float targets[SERVO_COUNT];

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    targets[servo] = servoMid(SERVO_SPECS[servo]);
  }

  targets[SERVO_HEAD] = anim::SLEEP_HEAD_DOWN;
  targets[SERVO_NECK] = mid;
  targets[SERVO_HAND_RIGHT] = handRightDown;
  targets[SERVO_HAND_LEFT] = handLeftDown;

  servoMoveAllSmoothTo(targets);
}

void bootRunSleepInertia() {
  const uint32_t startMs = millis();
  startWakeup(startMs, bootSleepInertiaUsesServos());
  setEyeMode(EyeMode::Wakeup, startMs);

  while (!wakeupFinished()) {
    const uint32_t now = millis();
    updateWakeup(now);
    updateEyes(now);
    delay(kFrameMs);
  }
}
