#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/eyes.h"
#include "display/oled.h"
#include "display/oled_internal.h"

Adafruit_SSD1306 display(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  -1
);

bool oledAvailable = false;

void showOledText(
  const char* line1,
  const char* line2
) {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 4);
  display.println(line1);

  if (line2 != nullptr) {
    display.setCursor(0, 18);
    display.println(line2);
  }

  display.display();
}

void showIdleScreen() {
  startEyes();
}

void blankOled() {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();
  display.display();
}

void sleepOled() {
  if (!oledAvailable) {
    return;
  }

  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void wakeOled() {
  if (!oledAvailable) {
    return;
  }

  display.ssd1306_command(SSD1306_DISPLAYON);
}
