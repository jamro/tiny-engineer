#include "display/eyes/modes/dead.h"

#include <Arduino.h>

#include "animation/dead.h"
#include "animation/error.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

constexpr int16_t FAIL_BASE_LEFT_H = 9;
constexpr int16_t FAIL_BASE_RIGHT_H = 11;
constexpr int16_t FAIL_JITTER_MAX = 3;
constexpr int16_t SQUEEZE_HEIGHT = 2;

constexpr uint32_t FLICKER_LO_MS = 40;
constexpr uint32_t FLICKER_HI_MS = 120;
constexpr uint32_t DENSE_FLICKER_LO_MS = 28;
constexpr uint32_t DENSE_FLICKER_HI_MS = 70;
constexpr uint32_t BLANK_LO_MS = 30;
constexpr uint32_t BLANK_HI_MS = 80;

constexpr uint32_t BLANK_CHANCE_MILD_PERCENT = 3;
constexpr uint32_t BLANK_CHANCE_NORMAL_PERCENT = 10;
constexpr uint32_t BLANK_CHANCE_DENSE_PERCENT = 22;

uint32_t g_nextFlickerMs = 0;
uint32_t g_blankUntilMs = 0;
int16_t g_leftH = FAIL_BASE_LEFT_H;
int16_t g_rightH = FAIL_BASE_RIGHT_H;
bool g_squeezeCaptured = false;
int16_t g_squeezeFromLeft = FAIL_BASE_LEFT_H;
int16_t g_squeezeFromRight = FAIL_BASE_RIGHT_H;

int16_t jitterHeight(int16_t base) {
  const int16_t delta =
    static_cast<int16_t>(esp_random() % (FAIL_JITTER_MAX * 2u + 1u))
    - FAIL_JITTER_MAX;
  const int16_t h = static_cast<int16_t>(base + delta);
  if (h < 3) {
    return 3;
  }
  if (h > 14) {
    return 14;
  }
  return h;
}

void scheduleNextFlicker(uint32_t now, bool dense) {
  g_nextFlickerMs = now + (dense
    ? anim::randRangeMs(DENSE_FLICKER_LO_MS, DENSE_FLICKER_HI_MS)
    : anim::randRangeMs(FLICKER_LO_MS, FLICKER_HI_MS));
}

uint32_t blankChancePercent(bool audioOn, bool dense) {
  if (dense) {
    return BLANK_CHANCE_DENSE_PERCENT;
  }
  if (audioOn) {
    return BLANK_CHANCE_NORMAL_PERCENT;
  }
  return BLANK_CHANCE_MILD_PERCENT;
}

void applyFailLook(const Eye& leftBase, const Eye& rightBase) {
  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    leftBase.x,
    leftBase.width,
    g_leftH
  );
  right = eyes::eyeWithHeight(
    rightBase.x,
    rightBase.width,
    g_rightH
  );
  left.y += 1;
  right.y += 1;
}

void updateFlickerWarning(uint32_t now) {
  const bool audioOn = errorAudioStarted();
  const uint32_t audioElapsed = errorAudioElapsed(now);
  const bool dense = audioOn
    && audioElapsed >= ERROR_AUDIO_PROBLEM_END_MS;

  if (now < g_blankUntilMs) {
    g_leftH = 1;
    g_rightH = 1;
  } else if (now >= g_nextFlickerMs) {
    g_leftH = jitterHeight(FAIL_BASE_LEFT_H);
    g_rightH = jitterHeight(FAIL_BASE_RIGHT_H);

    if (anim::randChance(blankChancePercent(audioOn, dense))) {
      g_blankUntilMs = now + anim::randRangeMs(BLANK_LO_MS, BLANK_HI_MS);
      g_leftH = 1;
      g_rightH = 1;
    }

    scheduleNextFlicker(now, dense);
  }

  applyFailLook(eyes::DEFAULT_LEFT, eyes::DEFAULT_RIGHT);
}

void updateSqueezeEyes(uint32_t now) {
  if (!g_squeezeCaptured) {
    g_squeezeFromLeft = g_leftH;
    g_squeezeFromRight = g_rightH;
    g_squeezeCaptured = true;
  }

  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, deadSqueezeStartedMs(), DEAD_SQUEEZE_MS)
  );
  g_leftH = static_cast<int16_t>(
    anim::lerp(
      static_cast<float>(g_squeezeFromLeft),
      static_cast<float>(SQUEEZE_HEIGHT),
      t
    ) + 0.5f
  );
  g_rightH = static_cast<int16_t>(
    anim::lerp(
      static_cast<float>(g_squeezeFromRight),
      static_cast<float>(SQUEEZE_HEIGHT),
      t
    ) + 0.5f
  );

  applyFailLook(eyes::DEFAULT_LEFT, eyes::DEFAULT_RIGHT);
}

void updatePauseEyes() {
  g_leftH = SQUEEZE_HEIGHT;
  g_rightH = SQUEEZE_HEIGHT;
  applyFailLook(eyes::DEFAULT_LEFT, eyes::DEFAULT_RIGHT);
}

}  // namespace

void startDeadEyes(uint32_t now) {
  blinkSetOpenAmount(1.0f);
  g_nextFlickerMs = now;
  g_blankUntilMs = 0;
  g_leftH = FAIL_BASE_LEFT_H;
  g_rightH = FAIL_BASE_RIGHT_H;
  g_squeezeCaptured = false;
  g_squeezeFromLeft = FAIL_BASE_LEFT_H;
  g_squeezeFromRight = FAIL_BASE_RIGHT_H;
}

void updateDeadEyes(uint32_t now) {
  if (deadShowingX()) {
    mutableLeftEye() = eyes::DEFAULT_LEFT;
    mutableRightEye() = eyes::DEFAULT_RIGHT;
    return;
  }

  if (deadIsSqueezing()) {
    updateSqueezeEyes(now);
    return;
  }

  if (deadIsPausedShut()) {
    updatePauseEyes();
    return;
  }

  updateFlickerWarning(now);
}
