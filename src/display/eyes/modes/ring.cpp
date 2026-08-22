#include "display/eyes/modes/ring.h"

#include "display/eyes.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"

void startRingEyes(uint32_t now) {
  (void)now;
}

void updateRingEyes(uint32_t now) {
  (void)now;

  Eye& left = mutableLeftEye();
  Eye& right = mutableRightEye();

  left = eyes::eyeWithHeight(eyes::DEFAULT_LEFT.x, eyes::DEFAULT_LEFT.width, 9);
  right = eyes::eyeWithHeight(
    eyes::DEFAULT_RIGHT.x,
    eyes::DEFAULT_RIGHT.width,
    9
  );
}
