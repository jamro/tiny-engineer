#include "display/eyes/modes/typing.h"

#include "animation/util.h"
#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

namespace {

bool g_typingLookRight = true;
uint32_t g_typingNextFlipMs = 0;

}  // namespace

void startTypingEyes(uint32_t now) {
  g_typingLookRight = anim::randChance(50);
  g_typingNextFlipMs = now + anim::randRangeMs(120, 260);
}

void updateTypingEyes(uint32_t now) {
  if (now >= g_typingNextFlipMs) {
    g_typingLookRight = !g_typingLookRight;
    g_typingNextFlipMs = now + anim::randRangeMs(120, 260);
  }

  const int16_t gx = g_typingLookRight ? 2 : -2;

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_LEFT.x + gx),
    eyes::DEFAULT_LEFT.width,
    12
  );
  right = eyes::eyeWithHeight(
    (int16_t)(eyes::DEFAULT_RIGHT.x + gx),
    eyes::DEFAULT_RIGHT.width,
    12
  );
  left.y += 2;
  right.y += 2;
}
