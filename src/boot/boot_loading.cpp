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
#include "settings/settings.h"

namespace {

constexpr uint32_t kFrameMs = 16;

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
  float targets[SERVO_COUNT];

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    targets[servo] = servoNormToDeg(servo, 0.0f);
  }

  targets[SERVO_HEAD] = servoNormToDeg(SERVO_HEAD, anim::SLEEP_HEAD_DOWN);
  targets[SERVO_NECK] = servoNormToDeg(SERVO_NECK, 0.0f);
  targets[SERVO_HAND_RIGHT] = servoNormToDeg(SERVO_HAND_RIGHT, -1.0f);
  targets[SERVO_HAND_LEFT] = servoNormToDeg(SERVO_HAND_LEFT, 1.0f);

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
