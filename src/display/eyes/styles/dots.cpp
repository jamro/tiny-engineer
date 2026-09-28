#include "display/eyes/styles/eye_style.h"

#include <Adafruit_SSD1306.h>

#include "animation/dead.h"
#include "display/eyes/core/blink.h"
#include "display/eyes/core/constants.h"
#include "display/eyes/core/draw.h"
#include "display/eyes/core/util.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "pins.h"

namespace {

constexpr int16_t kDotSize = 12;

// 12×12 filled circle. Bit 11 is the leftmost pixel.
constexpr uint16_t kDotMask[kDotSize] = {
  0b000011110000,
  0b001111111100,
  0b011111111110,
  0b011111111110,
  0b111111111111,
  0b111111111111,
  0b111111111111,
  0b111111111111,
  0b011111111110,
  0b011111111110,
  0b001111111100,
  0b000011110000,
};

bool dotMaskAt(int16_t x, int16_t y) {
  if (x < 0 || y < 0 || x >= kDotSize || y >= kDotSize) {
    return false;
  }

  return (kDotMask[y] & (1u << (kDotSize - 1 - x))) != 0;
}

int16_t dotDiameter(int16_t renderedHeight) {
  if (renderedHeight <= 0 || eyes::DEFAULT_LEFT.height <= 0) {
    return 0;
  }

  const int16_t d = (int16_t)(
    (int32_t)kDotSize * renderedHeight / eyes::DEFAULT_LEFT.height
  );

  if (d > OLED_HEIGHT) {
    return OLED_HEIGHT;
  }

  return d;
}

void drawScaledDot(const Eye& rendered) {
  const int16_t d = dotDiameter(rendered.height);
  if (d <= 0 || rendered.width <= 0) {
    return;
  }

  const int16_t cx = (int16_t)(rendered.x + rendered.width / 2);
  const int16_t cy = (int16_t)(rendered.y + rendered.height / 2);
  const int16_t x0 = (int16_t)(cx - d / 2);
  const int16_t y0 = (int16_t)(cy - d / 2);

  for (int16_t y = 0; y < d; y++) {
    const int16_t sy =
      (int16_t)(((int32_t)y * kDotSize + kDotSize / 2) / d);

    for (int16_t x = 0; x < d; x++) {
      const int16_t sx =
        (int16_t)(((int32_t)x * kDotSize + kDotSize / 2) / d);

      if (!dotMaskAt(sx, sy)) {
        continue;
      }

      display.drawPixel(
        (int16_t)(x0 + x),
        (int16_t)(y0 + y),
        SSD1306_WHITE
      );
    }
  }
}

Eye dotBox(const Eye& eye) {
  const int16_t cx = (int16_t)(eye.x + eye.width / 2);
  const int16_t cy = (int16_t)(eye.y + eye.height / 2);

  return {
    (int16_t)(cx - kDotSize / 2),
    (int16_t)(cy - kDotSize / 2),
    kDotSize,
    kDotSize
  };
}

void drawDots(
  EyeMode mode,
  AnimationId /*animation*/,
  uint32_t /*modeStartedMs*/,
  uint32_t /*now*/,
  EyeStyleSleepPhase /*sleepPhase*/
) {
  if (mode == EyeMode::Dead && deadShowingX()) {
    drawDeadXEyes(dotBox(leftEye()), dotBox(rightEye()));
    return;
  }

  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();
  drawScaledDot(eyes::renderEye(leftEye(), blinkOpenAmount()));
  drawScaledDot(eyes::renderEye(rightEye(), blinkOpenAmount()));
  display.display();
}

}  // namespace

extern const EyeStyleRenderer kDotsEyeStyle = {
  "dots",
  true,
  true,
  drawDots,
};
