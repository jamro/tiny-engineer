#include "display/eyes/modes/abort.h"

#include <Arduino.h>

#include "animation/abort.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

uint32_t g_abortEyesStartedMs = 0;

void applyAbortBlink(
  uint32_t audioElapsed,
  int16_t& leftHeight,
  int16_t& rightHeight
) {
  constexpr uint32_t BLINK_START_MS = 1180;
  constexpr uint32_t BLINK_END_MS = 1320;

  if (audioElapsed < BLINK_START_MS || audioElapsed >= BLINK_END_MS) {
    return;
  }

  const uint32_t mid = (BLINK_START_MS + BLINK_END_MS) / 2;
  const uint32_t half = (BLINK_END_MS - BLINK_START_MS) / 2;
  const float raw = audioElapsed < mid
    ? (float)(audioElapsed - BLINK_START_MS) / (float)half
    : (float)(BLINK_END_MS - audioElapsed) / (float)half;
  const float closed = anim::easeInOutCubic(constrain(raw, 0.0f, 1.0f));
  const int16_t height = (int16_t)(16.0f - 12.0f * closed);

  leftHeight = height;
  rightHeight = height;
}

}  // namespace

void startAbortEyes(uint32_t now) {
  g_abortEyesStartedMs = now;
}

void updateAbortEyes(uint32_t now) {
  int16_t xOffset = 0;
  int16_t yOffset = -2;
  int16_t leftHeight = 17;
  int16_t rightHeight = 17;

  if (abortAudioStarted()) {
    const uint32_t audioElapsed = abortAudioElapsed(now);

    if (audioElapsed < ABORT_AUDIO_FINE_END_MS) {
      xOffset = 0;
      yOffset = -3;
      leftHeight = 17;
      rightHeight = 17;
    } else if (audioElapsed < ABORT_AUDIO_DIDNT_WANT_END_MS) {
      xOffset = 5;
      yOffset = -1;
      leftHeight = 11;
      rightHeight = 15;
    } else if (audioElapsed < ABORT_AUDIO_FINISH_END_MS) {
      xOffset = -4;
      yOffset = 0;
      leftHeight = 13;
      rightHeight = 10;
    } else if (audioElapsed < ABORT_AUDIO_END_MS) {
      xOffset = 3;
      yOffset = -2;
      leftHeight = 10;
      rightHeight = 14;
    } else {
      leftHeight = 15;
      rightHeight = 15;
      yOffset = -1;
    }

    applyAbortBlink(audioElapsed, leftHeight, rightHeight);
  } else {
    const uint32_t prepElapsed = now - g_abortEyesStartedMs;
    if (prepElapsed > 260) {
      xOffset = 2;
      leftHeight = 16;
      rightHeight = 17;
    }
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + xOffset),
    eyes::DEFAULT_LEFT.width,
    leftHeight
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + xOffset),
    eyes::DEFAULT_RIGHT.width,
    rightHeight
  );
  left.y += yOffset;
  right.y += yOffset;
}
