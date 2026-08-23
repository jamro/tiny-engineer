#include <Arduino.h>
#include "esp32-hal-rgb-led.h"

#include "animation.h"
#include "network/wifi_connect.h"
#include "pins.h"
#include "hardware/rgb.h"

namespace {

constexpr uint8_t RGB_ANIM_WHITE = 255;
constexpr uint8_t RGB_ANIM_RED = 255;
constexpr uint32_t RGB_TRANSITION_MS = 1000;

constexpr uint8_t RGB_PROVISIONING_B = 64;

uint8_t g_startR = 0;
uint8_t g_startG = 0;
uint8_t g_startB = 0;
uint8_t g_targetR = 0;
uint8_t g_targetG = 0;
uint8_t g_targetB = 0;
uint8_t g_currentR = 0;
uint8_t g_currentG = 0;
uint8_t g_currentB = 0;
uint32_t g_transitionStartMs = 0;
bool g_transitionActive = false;
bool g_twoPhaseTransition = false;

uint8_t lerpChannel(uint8_t from, uint8_t to, float t) {
  return static_cast<uint8_t>(from + (to - from) * t);
}

float easeInOut(float t) {
  return t * t * (3.0f - 2.0f * t);
}

bool isOff(uint8_t r, uint8_t g, uint8_t b) {
  return r == 0 && g == 0 && b == 0;
}

bool sameColor(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
  return r1 == r2 && g1 == g2 && b1 == b2;
}

void writeRgb(uint8_t r, uint8_t g, uint8_t b) {
  // Waveshare ESP32-C3-Zero WS2812 uses RGB byte order (not GRB).
  rgbLedWriteOrdered(RGB_LED_PIN, LED_COLOR_ORDER_RGB, r, g, b);
  g_currentR = r;
  g_currentG = g;
  g_currentB = b;
}

void resolveAnimationTarget(AnimationId id, uint8_t& r, uint8_t& g, uint8_t& b) {
  switch (id) {
    case AnimationId::Typing:
    case AnimationId::Reading:
    case AnimationId::Thinking:
    case AnimationId::Welcome:
    case AnimationId::Ring:
      r = RGB_ANIM_WHITE;
      g = RGB_ANIM_WHITE;
      b = RGB_ANIM_WHITE;
      break;
    case AnimationId::Attention:
    case AnimationId::Error:
    case AnimationId::Abort:
      r = RGB_ANIM_RED;
      g = 0;
      b = 0;
      break;
    case AnimationId::None:
    default:
      r = 0;
      g = 0;
      b = 0;
      break;
  }
}

bool sameTarget(uint8_t r, uint8_t g, uint8_t b) {
  return r == g_targetR && g == g_targetG && b == g_targetB;
}

void applyTransition(uint32_t nowMs) {
  if (!g_transitionActive) {
    return;
  }

  const uint32_t elapsed =
    (nowMs >= g_transitionStartMs) ? (nowMs - g_transitionStartMs) : 0;
  if (elapsed >= RGB_TRANSITION_MS) {
    writeRgb(g_targetR, g_targetG, g_targetB);
    g_transitionActive = false;
    g_twoPhaseTransition = false;
    return;
  }

  if (g_twoPhaseTransition) {
    const uint32_t halfMs = RGB_TRANSITION_MS / 2;
    if (elapsed < halfMs) {
      const float t = easeInOut(static_cast<float>(elapsed) / static_cast<float>(halfMs));
      writeRgb(
        lerpChannel(g_startR, 0, t),
        lerpChannel(g_startG, 0, t),
        lerpChannel(g_startB, 0, t)
      );
      return;
    }

    const float t = easeInOut(
      static_cast<float>(elapsed - halfMs) / static_cast<float>(halfMs)
    );
    writeRgb(
      lerpChannel(0, g_targetR, t),
      lerpChannel(0, g_targetG, t),
      lerpChannel(0, g_targetB, t)
    );
    return;
  }

  const float t = easeInOut(
    static_cast<float>(elapsed) / static_cast<float>(RGB_TRANSITION_MS)
  );
  writeRgb(
    lerpChannel(g_startR, g_targetR, t),
    lerpChannel(g_startG, g_targetG, t),
    lerpChannel(g_startB, g_targetB, t)
  );
}

}  // namespace

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  writeRgb(r, g, b);
  g_startR = r;
  g_startG = g;
  g_startB = b;
  g_targetR = r;
  g_targetG = g;
  g_targetB = b;
  g_transitionActive = false;
  g_twoPhaseTransition = false;
}

void setRgbForAnimation(AnimationId id, uint32_t nowMs) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  resolveAnimationTarget(id, r, g, b);

  if (sameTarget(r, g, b) && !g_transitionActive) {
    return;
  }

  g_startR = g_currentR;
  g_startG = g_currentG;
  g_startB = g_currentB;
  g_targetR = r;
  g_targetG = g;
  g_targetB = b;

  const bool startOff = isOff(g_startR, g_startG, g_startB);
  const bool targetOff = isOff(g_targetR, g_targetG, g_targetB);
  g_twoPhaseTransition =
    !startOff &&
    !targetOff &&
    !sameColor(g_startR, g_startG, g_startB, g_targetR, g_targetG, g_targetB);

  g_transitionStartMs = nowMs;
  g_transitionActive = true;
  applyTransition(nowMs);
}

void updateRgb(uint32_t nowMs) {
  static bool wasProvisioning = false;

  if (wifiProvisioningMode()) {
    if (g_currentR != 0 || g_currentG != 0 || g_currentB != RGB_PROVISIONING_B) {
      setRgb(0, 0, RGB_PROVISIONING_B);
    }
    wasProvisioning = true;
    return;
  }

  if (wasProvisioning) {
    wasProvisioning = false;
    setRgbForAnimation(getAnimation(), nowMs);
  }

  applyTransition(nowMs);
}

void runRgbTest() {
  Serial.println("RGB test");

  setRgb(64, 0, 0);
  delay(300);

  setRgb(0, 64, 0);
  delay(300);

  setRgb(0, 0, 64);
  delay(300);

  setRgb(32, 32, 32);
  delay(300);

  setRgb(0, 0, 0);

  Serial.println("RGB OK");
}
