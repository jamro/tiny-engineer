#include <Arduino.h>

#include "animation/util.h"
#include "eyes.h"
#include "oled.h"

namespace {

constexpr int EYE_CORNER_RADIUS = 3;
constexpr float BLINK_CLOSED_AMOUNT = 0.12f;
constexpr uint32_t REDRAW_INTERVAL_MS = 33;
constexpr uint32_t DOUBLE_BLINK_CHANCE_PERCENT = 22;

enum class BlinkPhase {
  Idle,
  Closing,
  Closed,
  Opening
};

Eye g_leftEye = {20, 9, 24, 14};
Eye g_rightEye = {84, 9, 24, 14};

bool g_eyesActive = false;
BlinkPhase g_blinkPhase = BlinkPhase::Idle;
float g_openAmount = 1.0f;
uint32_t g_phaseStartedMs = 0;
uint32_t g_phaseDurationMs = 0;
uint32_t g_nextBlinkMs = 0;
uint32_t g_lastDrawMs = 0;
bool g_forceRedraw = false;
uint8_t g_blinksInSequence = 1;
uint8_t g_blinksDone = 0;

Eye renderEye(const Eye& base, float openAmount) {
  const int16_t renderHeight =
    (int16_t)(base.height * openAmount);

  if (renderHeight <= 0) {
    return {base.x, base.y, base.width, 0};
  }

  return {
    base.x,
    (int16_t)(base.y + (base.height - renderHeight) / 2),
    base.width,
    renderHeight
  };
}

float phaseProgress(uint32_t now, uint32_t durationMs) {
  if (durationMs == 0) {
    return 1.0f;
  }

  const uint32_t elapsed = now - g_phaseStartedMs;

  if (elapsed >= durationMs) {
    return 1.0f;
  }

  return (float)elapsed / (float)durationMs;
}

void scheduleNextBlink(uint32_t now) {
  g_nextBlinkMs = now + anim::randRangeMs(2500, 5500);
}

void beginClosing(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closing;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(65, 120);
  g_forceRedraw = true;
}

void beginClosed(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closed;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(35, 90);
  g_forceRedraw = true;
}

void beginOpening(uint32_t now) {
  g_blinkPhase = BlinkPhase::Opening;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(75, 130);
  g_forceRedraw = true;
}

void beginIdle(uint32_t now) {
  g_blinkPhase = BlinkPhase::Idle;
  g_phaseStartedMs = now;
  g_forceRedraw = true;
}

void startBlinkSequence(uint32_t now) {
  g_blinksInSequence =
    anim::randChance(DOUBLE_BLINK_CHANCE_PERCENT) ? 2 : 1;
  g_blinksDone = 0;
  beginClosing(now);
}

void finishBlinkSequence(uint32_t now) {
  g_blinksDone = 0;
  g_blinksInSequence = 1;
  beginIdle(now);
  scheduleNextBlink(now);
}

void advanceBlinkPhase(uint32_t now) {
  switch (g_blinkPhase) {
    case BlinkPhase::Idle:
      if (now >= g_nextBlinkMs) {
        startBlinkSequence(now);
      }
      break;

    case BlinkPhase::Closing: {
      const float t = anim::easeInOutCubic(
        phaseProgress(now, g_phaseDurationMs)
      );
      g_openAmount = 1.0f - t * (1.0f - BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = BLINK_CLOSED_AMOUNT;
        beginClosed(now);
      }
      break;
    }

    case BlinkPhase::Closed:
      if (now - g_phaseStartedMs >= g_phaseDurationMs) {
        beginOpening(now);
      }
      break;

    case BlinkPhase::Opening: {
      const float t = anim::easeInOutCubic(
        phaseProgress(now, g_phaseDurationMs)
      );
      g_openAmount =
        BLINK_CLOSED_AMOUNT + t * (1.0f - BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = 1.0f;
        g_blinksDone++;

        if (g_blinksDone < g_blinksInSequence) {
          beginIdle(now);
          g_nextBlinkMs = now + anim::randRangeMs(90, 220);
        } else {
          finishBlinkSequence(now);
        }
      }
      break;
    }
  }
}

void drawCurrentEyes() {
  const Eye left = renderEye(g_leftEye, g_openAmount);
  const Eye right = renderEye(g_rightEye, g_openAmount);
  drawEyes(left, right, EYE_CORNER_RADIUS);
}

}  // namespace

void startEyes() {
  g_eyesActive = true;
  g_blinksInSequence = 1;
  g_blinksDone = 0;
  g_openAmount = 1.0f;
  g_lastDrawMs = 0;
  g_forceRedraw = true;
  beginIdle(millis());
  g_nextBlinkMs = millis() + anim::randRangeMs(800, 2000);
  drawCurrentEyes();
  g_lastDrawMs = millis();
  g_forceRedraw = false;
}

void stopEyes() {
  g_eyesActive = false;
}

void updateEyes(uint32_t now) {
  if (!g_eyesActive) {
    return;
  }

  advanceBlinkPhase(now);

  if (!g_forceRedraw &&
      (now - g_lastDrawMs) < REDRAW_INTERVAL_MS) {
    return;
  }

  drawCurrentEyes();
  g_lastDrawMs = now;
  g_forceRedraw = false;
}

bool eyesActive() {
  return g_eyesActive;
}

const Eye& leftEye() {
  return g_leftEye;
}

const Eye& rightEye() {
  return g_rightEye;
}

Eye& mutableLeftEye() {
  return g_leftEye;
}

Eye& mutableRightEye() {
  return g_rightEye;
}
