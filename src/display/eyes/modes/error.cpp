#include "display/eyes/modes/error.h"

#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/internal.h"
#include "display/eyes/core/util.h"

void startErrorEyes(uint32_t now) {
  (void)now;
}

void updateErrorEyes(uint32_t now) {
  const uint32_t elapsed = now - eyes::modeStartedMs();
  const bool opening = elapsed < 2400;
  const bool glanceUser =
    !opening && ((elapsed - 2400) / 1800) % 2u == 1u;
  const int16_t xOffset = glanceUser ? 0 : -4;
  const int16_t leftHeight = opening ? 11 : (glanceUser ? 13 : 11);
  const int16_t rightHeight = opening ? 15 : (glanceUser ? 14 : 15);

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
  left.y += 1;
  right.y -= 1;
}
