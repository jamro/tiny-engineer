#include "display/eyes/modes/idle.h"

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

int16_t g_gazeFromX = 0;
int16_t g_gazeFromY = 0;
int16_t g_gazeX = 0;
int16_t g_gazeY = 0;
int16_t g_targetGazeX = 0;
int16_t g_targetGazeY = 0;
uint32_t g_gazeMoveStartMs = 0;
uint32_t g_gazeMoveDurationMs = 0;
uint32_t g_nextGazeShiftMs = 0;

}  // namespace

void startIdleEyes(uint32_t now) {
  g_gazeX = 0;
  g_gazeY = 0;
  g_targetGazeX = 0;
  g_targetGazeY = 0;
  g_gazeMoveStartMs = now;
  g_gazeMoveDurationMs = 0;
  g_nextGazeShiftMs = now + anim::randRangeMs(4000, 8000);
}

void updateIdleEyes(uint32_t now) {
  if (now >= g_nextGazeShiftMs) {
    g_gazeFromX = g_gazeX;
    g_gazeFromY = g_gazeY;
    g_targetGazeX = (int16_t)anim::randRangeMs(0, 4) - 2;
    g_targetGazeY = (int16_t)anim::randRangeMs(0, 2) - 1;
    g_gazeMoveStartMs = now;
    g_gazeMoveDurationMs = anim::randRangeMs(300, 400);
    g_nextGazeShiftMs = now + anim::randRangeMs(4000, 8000);
  }

  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, g_gazeMoveStartMs, g_gazeMoveDurationMs)
  );
  g_gazeX = eyes::lerpInt(g_gazeFromX, g_targetGazeX, t);
  g_gazeY = eyes::lerpInt(g_gazeFromY, g_targetGazeY, t);

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = {
    (int16_t)(eyes::DEFAULT_LEFT.x + g_gazeX),
    (int16_t)(eyes::DEFAULT_LEFT.y + g_gazeY),
    eyes::DEFAULT_LEFT.width,
    eyes::DEFAULT_LEFT.height
  };
  right = {
    (int16_t)(eyes::DEFAULT_RIGHT.x + g_gazeX),
    (int16_t)(eyes::DEFAULT_RIGHT.y + g_gazeY),
    eyes::DEFAULT_RIGHT.width,
    eyes::DEFAULT_RIGHT.height
  };
}
