#include "boot_loading.h"

#include <Arduino.h>
#include <cmath>
#include <cstring>

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/oled.h"
#include "servos.h"
#include "servo_wrapper.h"
#include "settings.h"

namespace {

constexpr uint32_t kOpenDurationMs = 2000;
constexpr uint32_t kTotalDurationMs = 5500;
constexpr uint32_t kBlink1AtMs = 2400;
constexpr uint32_t kBlink2AtMs = 3800;
constexpr uint32_t kFrameMs = 16;

// Head: mid - 20° (chin down), then slow rise to mid. Neck: slight side wave only.
constexpr float kHeadAsleepOffsetDeg = -20.0f;
constexpr float kNeckWaveAmpDeg = 4.0f;
constexpr float kWavePeriodMs = 2800.0f;
constexpr float kServoSpeedDegS = 35.0f;
constexpr float kTwoPi = 6.28318530718f;

float headMid() {
  return servoMid(SERVO_SPECS[SERVO_HEAD]);
}

float neckMid() {
  return servoMid(SERVO_SPECS[SERVO_NECK]);
}

void applyHeadNeck(
  float headDeg,
  float neckDeg
) {
  servoAt(SERVO_HEAD).setTarget(headDeg, kServoSpeedDegS);
  servoAt(SERVO_NECK).setTarget(neckDeg, kServoSpeedDegS);
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
  const float headAsleep = headMid() + kHeadAsleepOffsetDeg;
  const float mid = neckMid();
  const float handRightDown = SERVO_SPECS[SERVO_HAND_RIGHT].min;
  const float handLeftDown = SERVO_SPECS[SERVO_HAND_LEFT].max;

  for (int servo = 0; servo < SERVO_COUNT; servo++) {
    servoAt(servo).snapTo(servoMid(SERVO_SPECS[servo]));
  }

  servoAt(SERVO_HEAD).snapTo(headAsleep);
  servoAt(SERVO_NECK).snapTo(mid);
  servoAt(SERVO_HAND_RIGHT).snapTo(handRightDown);
  servoAt(SERVO_HAND_LEFT).snapTo(handLeftDown);
}

void bootRunSleepInertia() {
  const bool moveServos = bootSleepInertiaUsesServos();
  const uint32_t startMs = millis();
  const float headAsleep = headMid() + kHeadAsleepOffsetDeg;
  const float headTarget = headMid();
  const float neckTarget = neckMid();

  blinkBeginIdle(startMs);
  blinkSetNextBlinkMs(startMs + 60000UL);
  blinkSetOpenAmount(eyes::BLINK_CLOSED_AMOUNT);
  eyes::requestForceRedraw();

  bool blink1Armed = true;
  bool blink2Armed = true;
  bool openComplete = false;

  while (true) {
    const uint32_t now = millis();
    const uint32_t elapsed = now - startMs;

    if (elapsed >= kTotalDurationMs) {
      break;
    }

    if (elapsed < kOpenDurationMs) {
      const float t =
        static_cast<float>(elapsed) /
        static_cast<float>(kOpenDurationMs);
      const float openAmount = anim::lerp(
        eyes::BLINK_CLOSED_AMOUNT,
        1.0f,
        anim::easeInOutCubic(t)
      );
      blinkSetOpenAmount(openAmount);
      eyes::requestForceRedraw();
    } else if (!openComplete) {
      blinkSetOpenAmount(1.0f);
      eyes::requestForceRedraw();
      openComplete = true;
    }

    if (openComplete) {
      if (blink1Armed && elapsed >= kBlink1AtMs) {
        blinkSetNextBlinkMs(now);
        blink1Armed = false;
      }

      if (blink2Armed && elapsed >= kBlink2AtMs) {
        blinkSetNextBlinkMs(now);
        blink2Armed = false;
      }
    }

    if (moveServos) {
      const float riseT = anim::easeInOutCubic(
        static_cast<float>(elapsed) /
        static_cast<float>(kTotalDurationMs)
      );
      const float headDeg = anim::lerp(headAsleep, headTarget, riseT);

      // Sideways neck wave, amplitude fades out as head finishes rising.
      const float waveT =
        static_cast<float>(elapsed) / kWavePeriodMs;
      const float wave = sinf(waveT * kTwoPi);
      const float waveFade = 1.0f - riseT;
      const float neckDeg =
        neckTarget + wave * kNeckWaveAmpDeg * waveFade;

      applyHeadNeck(headDeg, neckDeg);
      updateAllServos();
    }

    updateEyes(now);
    delay(kFrameMs);
  }

  if (moveServos) {
    applyHeadNeck(headTarget, neckTarget);

    const uint32_t settleStart = millis();

    while (millis() - settleStart < 400UL) {
      updateAllServos();
      updateEyes(millis());
      delay(kFrameMs);
    }

    servoAt(SERVO_HEAD).stop();
    servoAt(SERVO_NECK).stop();
  }

  blinkSetOpenAmount(1.0f);
  blinkBeginIdle(millis());
  blinkSetNextBlinkMs(millis() + anim::randRangeMs(800, 2000));
  eyes::requestForceRedraw();
  updateEyes(millis());
}
