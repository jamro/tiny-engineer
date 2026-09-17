#include "display/eyes/styles/eye_style.h"

#include "animation/dead.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/draw.h"
#include "display/eyes/core/util.h"

namespace {

void drawClassic(
  EyeMode mode,
  AnimationId /*animation*/,
  uint32_t /*modeStartedMs*/,
  uint32_t /*now*/,
  EyeStyleSleepPhase /*sleepPhase*/
) {
  if (mode == EyeMode::Dead && deadShowingX()) {
    drawDeadXEyes(leftEye(), rightEye());
    return;
  }

  const Eye left = eyes::renderEye(leftEye(), blinkOpenAmount());
  const Eye right = eyes::renderEye(rightEye(), blinkOpenAmount());
  drawEyes(left, right, eyes::EYE_CORNER_RADIUS);
}

}  // namespace

extern const EyeStyleRenderer kClassicEyeStyle = {
  "classic",
  true,
  true,
  drawClassic,
};
