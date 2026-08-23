#include "display/eyes/core/draw.h"

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "display/oled.h"
#include "display/oled_internal.h"

namespace {

void fillRoundedEye(
  const Eye& eye,
  int cornerRadius
) {
  if (eye.width <= 0 || eye.height <= 0) {
    return;
  }

  const int radius = min(
    cornerRadius,
    min(eye.width / 2, eye.height / 2)
  );

  if (radius <= 0) {
    display.fillRect(
      eye.x,
      eye.y,
      eye.width,
      eye.height,
      SSD1306_WHITE
    );
    return;
  }

  display.fillRect(
    eye.x + radius,
    eye.y,
    eye.width - 2 * radius,
    eye.height,
    SSD1306_WHITE
  );

  display.fillRect(
    eye.x,
    eye.y + radius,
    eye.width,
    eye.height - 2 * radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + radius,
    eye.y + radius,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + eye.width - radius - 1,
    eye.y + radius,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + radius,
    eye.y + eye.height - radius - 1,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + eye.width - radius - 1,
    eye.y + eye.height - radius - 1,
    radius,
    SSD1306_WHITE
  );
}

}  // namespace

void drawEyes(
  const Eye& left,
  const Eye& right,
  int cornerRadius
) {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();
  fillRoundedEye(left, cornerRadius);
  fillRoundedEye(right, cornerRadius);
  display.display();
}
