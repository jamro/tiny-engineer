#include "display/eyes/modes/thinking.h"

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

namespace {

int8_t g_thinkSideX = 3;
bool g_thinkSquintLeft = true;
uint32_t g_nextThinkSquintFlipMs = 0;
uint32_t g_nextThinkIdeaMs = 0;
uint32_t g_thinkIdeaUntilMs = 0;

}  // namespace

void startThinkingEyes(uint32_t now) {
  g_thinkSideX = anim::randChance(50) ? 3 : -3;
  g_thinkSquintLeft = anim::randChance(50);
  g_nextThinkSquintFlipMs = now + anim::randRangeMs(2000, 4500);
  g_nextThinkIdeaMs = now + anim::randRangeMs(5000, 10000);
  g_thinkIdeaUntilMs = 0;
}

void updateThinkingEyes(uint32_t now) {
  if (now >= g_nextThinkSquintFlipMs) {
    g_thinkSquintLeft = !g_thinkSquintLeft;
    g_thinkSideX = (int8_t)(g_thinkSideX * -1);
    g_nextThinkSquintFlipMs = now + anim::randRangeMs(2000, 4500);
  }

  if (now >= g_nextThinkIdeaMs && g_thinkIdeaUntilMs == 0) {
    g_thinkIdeaUntilMs = now + 180;
    g_nextThinkIdeaMs = now + anim::randRangeMs(5000, 10000);
    eyes::requestForceRedraw();
  }

  const bool ideaActive =
    g_thinkIdeaUntilMs != 0 && now < g_thinkIdeaUntilMs;

  if (!ideaActive && g_thinkIdeaUntilMs != 0 &&
      now >= g_thinkIdeaUntilMs) {
    g_thinkIdeaUntilMs = 0;
  }

  const int16_t leftHeight = ideaActive
    ? 17
    : (g_thinkSquintLeft ? 11 : 14);
  const int16_t rightHeight = ideaActive
    ? 17
    : (g_thinkSquintLeft ? 14 : 11);

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + g_thinkSideX),
    eyes::DEFAULT_LEFT.width,
    leftHeight
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + g_thinkSideX),
    eyes::DEFAULT_RIGHT.width,
    rightHeight
  );
  left.y -= 2;
  right.y -= 2;
}
