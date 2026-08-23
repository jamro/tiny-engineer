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

constexpr float kHeadAsleepOffsetDeg = -8.0f;
constexpr float kNeckAsleepOffsetDeg = 5.0f;
constexpr float kHeadWaveAmpDeg = 4.0f;
constexpr float kNeckWaveAmpDeg = 5.0f;
constexpr float kWavePeriodMs = 3200.0f;
constexpr float kServoSpeedDegS = 40.0f;
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

void bootRunSleepInertia() {
  const bool moveServos = settingsWelcomeEnabled();
  const uint32_t startMs = millis();
  const float headBase = headMid() + kHeadAsleepOffsetDeg;
  const float neckBase = neckMid() + kNeckAsleepOffsetDeg;

  if (moveServos) {
    applyHeadNeck(headBase, neckBase);
  }

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
      const float waveT =
        static_cast<float>(elapsed) / kWavePeriodMs;
      const float wave = sinf(waveT * kTwoPi);
      const float settle =
        elapsed < kOpenDurationMs
          ? 1.0f
          : anim::lerp(
              1.0f,
              0.0f,
              anim::easeInOutCubic(
                static_cast<float>(elapsed - kOpenDurationMs) /
                static_cast<float>(kTotalDurationMs - kOpenDurationMs)
              )
            );

      const float headDeg =
        anim::lerp(headBase, headMid(), 1.0f - settle) +
        wave * kHeadWaveAmpDeg * settle;
      const float neckDeg =
        anim::lerp(neckBase, neckMid(), 1.0f - settle) +
        wave * kNeckWaveAmpDeg * settle;

      applyHeadNeck(headDeg, neckDeg);
      updateAllServos();
    }

    updateEyes(now);
    delay(kFrameMs);
  }

  if (moveServos) {
    applyHeadNeck(headMid(), neckMid());

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
