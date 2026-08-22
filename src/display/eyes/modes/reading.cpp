#include "display/eyes/modes/reading.h"

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

enum class ReadingScanPhase {
  Forward,
  Return
};

ReadingScanPhase g_readScanPhase = ReadingScanPhase::Forward;
int16_t g_readScanX = -5;
uint32_t g_readScanStartMs = 0;
uint32_t g_readScanDurationMs = 0;

}  // namespace

void startReadingEyes(uint32_t now) {
  g_readScanPhase = ReadingScanPhase::Forward;
  g_readScanX = -5;
  g_readScanStartMs = now;
  g_readScanDurationMs = anim::randRangeMs(1100, 1800);
}

void updateReadingEyes(uint32_t now) {
  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, g_readScanStartMs, g_readScanDurationMs)
  );

  if (g_readScanPhase == ReadingScanPhase::Forward) {
    g_readScanX = eyes::lerpInt(-5, 5, t);

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Return;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(120, 220);
    }
  } else {
    g_readScanX = eyes::lerpInt(5, -5, t);

    if (t >= 1.0f) {
      g_readScanPhase = ReadingScanPhase::Forward;
      g_readScanStartMs = now;
      g_readScanDurationMs = anim::randRangeMs(1100, 1800);
      blinkScheduleSoon(now, 80, 180);
    }
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + g_readScanX),
    eyes::DEFAULT_LEFT.width,
    eyes::DEFAULT_LEFT.height
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + g_readScanX),
    eyes::DEFAULT_RIGHT.width,
    eyes::DEFAULT_RIGHT.height
  );
  left.y += 1;
  right.y += 1;
}
