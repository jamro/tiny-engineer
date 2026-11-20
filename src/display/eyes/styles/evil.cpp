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

// Open silhouette taller than classic 14px so the scowl reads on 128×32.
// Blink scales this height; width stays near kEvilWidth for the inter-eye gap.
constexpr int16_t kEvilWidth = 42;
constexpr int16_t kEvilOpenHeight = 24;

float bump01(float t) {
  if (t <= 0.0f || t >= 1.0f) {
    return 0.0f;
  }
  return 4.0f * t * (1.0f - t);
}

// Left-eye unit test: nx=0 outer, nx=1 inner; ny=0 top, ny=1 bottom.
// Top and bottom share a slanted base (outer high → inner low). Depth bump
// opens the eye; a smaller notch digs the brow without collapsing fill.
bool insideEvilUnit(float nx, float ny) {
  if (nx < 0.0f || nx > 1.0f || ny < 0.0f || ny > 1.0f) {
    return false;
  }

  constexpr float kOuterY = 0.06f;
  constexpr float kInnerY = 0.50f;
  constexpr float kNotch = 0.10f;
  constexpr float kDepth = 0.72f;

  const float base = kOuterY + (kInnerY - kOuterY) * nx;
  const float lift = bump01(nx);
  float top = base + kNotch * lift;
  float bottom = base + kDepth * lift;

  if (bottom <= top) {
    return false;
  }

  // Sharp outer tip only — keep mid-eye thickness.
  if (nx < 0.08f) {
    const float tip = nx / 0.08f;
    const float mid = 0.5f * (top + bottom);
    top = mid + (top - mid) * tip;
    bottom = mid + (bottom - mid) * tip;
  }

  return ny >= top && ny <= bottom;
}

int16_t evilHeight(int16_t renderedHeight) {
  if (renderedHeight <= 0 || eyes::DEFAULT_LEFT.height <= 0) {
    return 0;
  }

  const int16_t h = (int16_t)(
    (int32_t)kEvilOpenHeight * renderedHeight / eyes::DEFAULT_LEFT.height
  );

  if (h > OLED_HEIGHT) {
    return OLED_HEIGHT;
  }

  return h;
}

void drawEvilSilhouette(const Eye& rendered, bool mirrorRight) {
  if (rendered.height <= 0 || rendered.width <= 0) {
    return;
  }

  const int16_t h = evilHeight(rendered.height);
  if (h <= 0) {
    return;
  }

  int16_t w = kEvilWidth;
  if (rendered.width > 0) {
    w = (int16_t)(
      (int32_t)kEvilWidth * rendered.width / eyes::DEFAULT_LEFT.width
    );
  }
  if (w < 4) {
    return;
  }
  if (w > OLED_WIDTH) {
    w = OLED_WIDTH;
  }

  const int16_t cx = (int16_t)(rendered.x + rendered.width / 2);
  const int16_t cy = (int16_t)(rendered.y + rendered.height / 2);
  const int16_t x0 = (int16_t)(cx - w / 2);
  const int16_t y0 = (int16_t)(cy - h / 2);

  for (int16_t y = 0; y < h; y++) {
    const float ny = (h <= 1) ? 0.5f : (float)y / (float)(h - 1);

    for (int16_t x = 0; x < w; x++) {
      float nx = (w <= 1) ? 0.5f : (float)x / (float)(w - 1);
      if (mirrorRight) {
        nx = 1.0f - nx;
      }

      if (!insideEvilUnit(nx, ny)) {
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

Eye evilBox(const Eye& eye) {
  const int16_t cx = (int16_t)(eye.x + eye.width / 2);
  const int16_t cy = (int16_t)(eye.y + eye.height / 2);

  return {
    (int16_t)(cx - kEvilWidth / 2),
    (int16_t)(cy - kEvilOpenHeight / 2),
    kEvilWidth,
    kEvilOpenHeight
  };
}

void drawEvil(
  EyeMode mode,
  AnimationId /*animation*/,
  uint32_t /*modeStartedMs*/,
  uint32_t /*now*/,
  EyeStyleSleepPhase /*sleepPhase*/
) {
  if (mode == EyeMode::Dead && deadShowingX()) {
    drawDeadXEyes(evilBox(leftEye()), evilBox(rightEye()));
    return;
  }

  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();
  drawEvilSilhouette(eyes::renderEye(leftEye(), blinkOpenAmount()), false);
  drawEvilSilhouette(eyes::renderEye(rightEye(), blinkOpenAmount()), true);
  display.display();
}

}  // namespace

extern const EyeStyleRenderer kEvilEyeStyle = {
  "evil",
  true,
  true,
  drawEvil,
};
