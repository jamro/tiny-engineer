#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/eyes.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "network/wifi_connect.h"

Adafruit_SSD1306 display(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  -1
);

bool oledAvailable = false;

namespace {

constexpr uint32_t kProvisionRotateMs = 3500;

uint32_t lastProvisionRotateMs = 0;
uint8_t provisionStep = 0;

}  // namespace

void showProvisioningOled(
  const char* line1,
  const char* line2
) {
  stopEyes();
  wakeOled();
  lastProvisionRotateMs = 0;
  showOledText(line1, line2);
}

void updateProvisioningOled(uint32_t nowMs) {
  if (!oledAvailable || !wifiProvisioningMode()) {
    return;
  }

  wakeOled();

  if (lastProvisionRotateMs != 0 &&
      nowMs - lastProvisionRotateMs < kProvisionRotateMs) {
    return;
  }

  lastProvisionRotateMs = nowMs;
  provisionStep = static_cast<uint8_t>((provisionStep + 1) % 2);

  if (provisionStep == 0) {
    showOledText("Join this WiFi", wifiApSsid());
  } else {
    showOledText("Then open", wifiApIpText());
  }
}

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
