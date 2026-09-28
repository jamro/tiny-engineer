#include "display/eyes/styles/eye_style.h"

#include <Adafruit_SSD1306.h>

#include "animation/dead.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/util.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "pins.h"

namespace {

constexpr int16_t kHalfWidth = OLED_WIDTH / 2;

int16_t coverWhiteHeight(int16_t renderedHeight) {
  if (renderedHeight <= 0 || eyes::DEFAULT_LEFT.height <= 0) {
    return 0;
  }

  const int32_t whiteH =
    (int32_t)OLED_HEIGHT * renderedHeight / eyes::DEFAULT_LEFT.height;

  if (whiteH >= OLED_HEIGHT) {
    return OLED_HEIGHT;
  }

  return (int16_t)whiteH;
}

int16_t coverTopMargin(const Eye& rendered, int16_t whiteH) {
  const int16_t black = (int16_t)(OLED_HEIGHT - whiteH);
  if (black <= 0) {
    return 0;
  }

  int16_t classicTop = rendered.y;
  int16_t classicBottom =
    (int16_t)(OLED_HEIGHT - (rendered.y + rendered.height));

  if (classicTop < 0) {
    classicTop = 0;
  }
  if (classicBottom < 0) {
    classicBottom = 0;
  }

  const int16_t denom = (int16_t)(classicTop + classicBottom);
  if (denom <= 0) {
    return (int16_t)(black / 2);
  }

  return (int16_t)((int32_t)black * classicTop / denom);
}

void fillCoverHalf(int16_t x, const Eye& rendered) {
  const int16_t whiteH = coverWhiteHeight(rendered.height);
  if (whiteH <= 0) {
    return;
  }

  display.fillRect(
    x,
    coverTopMargin(rendered, whiteH),
    kHalfWidth,
    whiteH,
    SSD1306_WHITE
  );
}

void drawCover(
  EyeMode mode,
  AnimationId /*animation*/,
  uint32_t /*modeStartedMs*/,
  uint32_t /*now*/,
  EyeStyleSleepPhase /*sleepPhase*/
) {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  if (mode == EyeMode::Dead && deadShowingX()) {
    display.display();
    return;
  }

  fillCoverHalf(0, eyes::renderEye(leftEye(), blinkOpenAmount()));
  fillCoverHalf(
    kHalfWidth,
    eyes::renderEye(rightEye(), blinkOpenAmount())
  );
  display.display();
}

}  // namespace

extern const EyeStyleRenderer kCoverEyeStyle = {
  "cover",
  true,
  true,
  drawCover,
};
