#include "display/eyes/core/blink.h"

#include "animation/util.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

namespace {

enum class BlinkPhase {
  Idle,
  Closing,
  Closed,
  Opening
};

BlinkPhase g_blinkPhase = BlinkPhase::Idle;
float g_openAmount = 1.0f;
uint32_t g_phaseStartedMs = 0;
uint32_t g_phaseDurationMs = 0;
uint32_t g_nextBlinkMs = 0;
uint8_t g_blinksInSequence = 1;
uint8_t g_blinksDone = 0;

void scheduleNextBlink(uint32_t now) {
  switch (eyes::currentEyeMode()) {
    case EyeMode::Typing:
      g_nextBlinkMs = now + anim::randRangeMs(1800, 4000);
      break;
    default:
      g_nextBlinkMs = now + anim::randRangeMs(2500, 5500);
      break;
  }
}

void beginClosing(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closing;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(65, 120);
  eyes::requestForceRedraw();
}

void beginClosed(uint32_t now) {
  g_blinkPhase = BlinkPhase::Closed;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(35, 90);
  eyes::requestForceRedraw();
}

void beginOpening(uint32_t now) {
  g_blinkPhase = BlinkPhase::Opening;
  g_phaseStartedMs = now;
  g_phaseDurationMs = anim::randRangeMs(75, 130);
  eyes::requestForceRedraw();
}

void startBlinkSequence(uint32_t now) {
  g_blinksInSequence =
    anim::randChance(eyes::DOUBLE_BLINK_CHANCE_PERCENT) ? 2 : 1;
  g_blinksDone = 0;
  beginClosing(now);
}

void finishBlinkSequence(uint32_t now) {
  g_blinksDone = 0;
  g_blinksInSequence = 1;
  blinkBeginIdle(now);
  scheduleNextBlink(now);
}

}  // namespace

void blinkBeginIdle(uint32_t now) {
  g_blinkPhase = BlinkPhase::Idle;
  g_phaseStartedMs = now;
  eyes::requestForceRedraw();
}

void blinkSetNextBlinkMs(uint32_t when) {
  g_nextBlinkMs = when;
}

void blinkSetOpenAmount(float amount) {
  g_openAmount = amount;
}

float blinkOpenAmount() {
  return g_openAmount;
}

void blinkResetCounters() {
  g_blinksInSequence = 1;
  g_blinksDone = 0;
}

void blinkAdvance(uint32_t now) {
  switch (g_blinkPhase) {
    case BlinkPhase::Idle:
      if (now >= g_nextBlinkMs) {
        startBlinkSequence(now);
      }
      break;

    case BlinkPhase::Closing: {
      const float t = anim::easeInOutCubic(
        eyes::phaseProgress(now, g_phaseStartedMs, g_phaseDurationMs)
      );
      g_openAmount = 1.0f - t * (1.0f - eyes::BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = eyes::BLINK_CLOSED_AMOUNT;
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
        eyes::phaseProgress(now, g_phaseStartedMs, g_phaseDurationMs)
      );
      g_openAmount =
        eyes::BLINK_CLOSED_AMOUNT + t * (1.0f - eyes::BLINK_CLOSED_AMOUNT);

      if (t >= 1.0f) {
        g_openAmount = 1.0f;
        g_blinksDone++;

        if (g_blinksDone < g_blinksInSequence) {
          blinkBeginIdle(now);
          g_nextBlinkMs = now + anim::randRangeMs(90, 220);
        } else {
          finishBlinkSequence(now);
        }
      }
      break;
    }
  }
}

bool blinkIsIdle() {
  return g_blinkPhase == BlinkPhase::Idle;
}

void blinkScheduleSoon(uint32_t now, uint32_t loMs, uint32_t hiMs) {
  if (blinkIsIdle()) {
    g_nextBlinkMs = now + anim::randRangeMs(loMs, hiMs);
  }
}

void blinkOnSleepOpenComplete(uint32_t now) {
  g_openAmount = 1.0f;
  blinkBeginIdle(now);
  scheduleNextBlink(now);
}
