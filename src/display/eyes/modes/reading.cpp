#include "display/eyes/modes/reading.h"

#include <Arduino.h>

#include "animation/constants.h"
#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"
#include "servo_wrapper.h"
#include "servos.h"

namespace {

constexpr int16_t READ_NECK_GAZE_PX = 6;
constexpr int16_t READ_SCAN_AMPLITUDE = 3;

enum class ReadingScanPhase {
  Forward,
  Return
};

ReadingScanPhase g_readScanPhase = ReadingScanPhase::Forward;
int16_t g_readScanOffset = -READ_SCAN_AMPLITUDE;
uint32_t g_readScanStartMs = 0;
uint32_t g_readScanDurationMs = 0;

float clampUnit(float value) {
  return constrain(value, -1.0f, 1.0f);
}

int16_t gazeXFromNeck() {
  const float neck = servoAt(SERVO_NECK).angle();
  const float norm = clampUnit(
    (neck - anim::READING_NECK_MID) / anim::READING_NECK_SWAY_DEG
  );
  return (int16_t)(norm * READ_NECK_GAZE_PX);
}

int16_t gazeYFromHead() {
  const float head = servoAt(SERVO_HEAD).angle();
  const float span = anim::READING_HEAD_HIGH - anim::READING_HEAD_LOW;
  if (span <= 0.0f) {
    return 2;
  }

  const float norm = clampUnit((head - anim::READING_HEAD_LOW) / span);
  return eyes::lerpInt(3, 1, norm);
}

void advanceReadingScan(uint32_t now) {
  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, g_readScanStartMs, g_readScanDurationMs)
  );

  if (g_readScanPhase == ReadingScanPhase::Forward) {
    g_readScanOffset = eyes::lerpInt(
      -READ_SCAN_AMPLITUDE,
      READ_SCAN_AMPLITUDE,
      t
    );

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Return;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(70, 140);
    }
  } else {
    g_readScanOffset = eyes::lerpInt(
      READ_SCAN_AMPLITUDE,
      -READ_SCAN_AMPLITUDE,
      t
    );

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Forward;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(900, 1500);
      blinkScheduleSoon(now, 80, 180);
    }
  }
}

}  // namespace

void startReadingEyes(uint32_t now) {
  g_readScanPhase = ReadingScanPhase::Forward;
  g_readScanOffset = -READ_SCAN_AMPLITUDE;
  g_readScanStartMs = now;
  g_readScanDurationMs = anim::randRangeMs(900, 1500);
}

void updateReadingEyes(uint32_t now) {
  advanceReadingScan(now);

  const int16_t gazeX = gazeXFromNeck() + g_readScanOffset;
  const int16_t gazeY = gazeYFromHead();

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + gazeX),
    eyes::DEFAULT_LEFT.width,
    eyes::DEFAULT_LEFT.height
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + gazeX),
    eyes::DEFAULT_RIGHT.width,
    eyes::DEFAULT_RIGHT.height
  );
  left.y += gazeY;
  right.y += gazeY;
}
