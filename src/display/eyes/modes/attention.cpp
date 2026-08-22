#include "display/eyes/modes/attention.h"

#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

void startAttentionEyes(uint32_t now) {
  (void)now;
}

void updateAttentionEyes(uint32_t now) {
  const uint32_t elapsed = now - eyes::modeStartedMs();
  const bool opening = elapsed < 2200;
  const int16_t height = opening ? 16 : 15;
  int16_t yOffset = opening ? -2 : -1;

  if (!opening && ((elapsed / 1400) % 2u) == 0u) {
    yOffset -= 1;
  }

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(
    eyes::DEFAULT_LEFT.x,
    eyes::DEFAULT_LEFT.width,
    height
  );
  right = eyes::eyeWithHeight(
    eyes::DEFAULT_RIGHT.x,
    eyes::DEFAULT_RIGHT.width,
    height
  );
  left.y += yOffset;
  right.y += yOffset;
}
