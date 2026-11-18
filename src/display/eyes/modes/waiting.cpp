#include "display/eyes/modes/waiting.h"

#include "animation/util.h"
#include "animation/waiting.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

namespace {

// The eyes look to the corner the head turns to. They get there first
// (a quick glance), then the head follows over about a second.
constexpr float GAZE_RANGE_X_PX = 5.0f;  // same full glance as error/attention
constexpr float GAZE_UP_PX = 3.0f;
constexpr float GAZE_DOWN_PX = 2.0f;
constexpr uint32_t GAZE_MOVE_MS = 180;

// Lids follow the gaze: a little wider looking up, a little lower looking down.
constexpr int16_t HEIGHT_UP = 15;
constexpr int16_t HEIGHT_DOWN = 12;

float g_fromX = 0.0f;
float g_fromY = 0.0f;
float g_toX = 0.0f;
float g_toY = 0.0f;
float g_x = 0.0f;
float g_y = 0.0f;
uint32_t g_gazeSinceMs = 0;
uint32_t g_moveStartMs = 0;

}  // namespace

void startWaitingEyes(uint32_t now) {
  g_fromX = 0.0f;
  g_fromY = 0.0f;
  g_toX = 0.0f;
  g_toY = 0.0f;
  g_x = 0.0f;
  g_y = 0.0f;
  g_gazeSinceMs = 0;
  g_moveStartMs = now;
}

void updateWaitingEyes(uint32_t now) {
  const WaitingGaze gaze = waitingGaze();
  if (gaze.sinceMs != g_gazeSinceMs) {
    g_gazeSinceMs = gaze.sinceMs;
    g_fromX = g_x;
    g_fromY = g_y;
    g_toX = gaze.x;
    g_toY = gaze.y;
    g_moveStartMs = now;
  }

  const float t = anim::easeInOutCubic(
    eyes::moveProgress(now, g_moveStartMs, GAZE_MOVE_MS)
  );
  g_x = anim::lerp(g_fromX, g_toX, t);
  g_y = anim::lerp(g_fromY, g_toY, t);

  // Screen x follows the neck sign: the robot's right is -x (see error eyes).
  const int16_t offsetX = (int16_t)(g_x * GAZE_RANGE_X_PX);
  const int16_t offsetY = (int16_t)(g_y >= 0.0f
    ? -g_y * GAZE_UP_PX
    : -g_y * GAZE_DOWN_PX);
  const int16_t height = g_y >= 0.0f
    ? eyes::lerpInt(eyes::DEFAULT_LEFT.height, HEIGHT_UP, g_y)
    : eyes::lerpInt(eyes::DEFAULT_LEFT.height, HEIGHT_DOWN, -g_y);

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + offsetX),
    eyes::DEFAULT_LEFT.width,
    height
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + offsetX),
    eyes::DEFAULT_RIGHT.width,
    height
  );
  left.y += offsetY;
  right.y += offsetY;
}
