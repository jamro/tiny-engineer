#include "display/eyes/modes/wakeup.h"

#include <Arduino.h>

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"

namespace {

constexpr uint32_t kOpenDurationMs = 2000;
constexpr uint32_t kBlink1AtMs = 2400;
constexpr uint32_t kBlink2AtMs = 3800;

bool g_blink1Armed = true;
bool g_blink2Armed = true;
bool g_openComplete = false;

}  // namespace

void startWakeupEyes(uint32_t now) {
  blinkBeginIdle(now);
  blinkSetNextBlinkMs(now + 60000UL);
  blinkSetOpenAmount(eyes::BLINK_CLOSED_AMOUNT);
  eyes::requestForceRedraw();

  g_blink1Armed = true;
  g_blink2Armed = true;
  g_openComplete = false;

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();
  left = eyes::DEFAULT_LEFT;
  right = eyes::DEFAULT_RIGHT;
}

void updateWakeupEyes(uint32_t now) {
  const uint32_t elapsed = now - eyes::modeStartedMs();

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
  } else if (!g_openComplete) {
    blinkSetOpenAmount(1.0f);
    eyes::requestForceRedraw();
    g_openComplete = true;
  }

  if (g_openComplete) {
    if (g_blink1Armed && elapsed >= kBlink1AtMs) {
      blinkSetNextBlinkMs(now);
      g_blink1Armed = false;
    }

    if (g_blink2Armed && elapsed >= kBlink2AtMs) {
      blinkSetNextBlinkMs(now);
      g_blink2Armed = false;
    }

    blinkAdvance(now);
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();
  left = eyes::DEFAULT_LEFT;
  right = eyes::DEFAULT_RIGHT;
}
