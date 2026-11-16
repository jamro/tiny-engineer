#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "serial_log.h"
#include "settings/settings.h"

bool i2cDeviceConnected(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

namespace {

// Sends the SSD1306 its full setup. Safe to repeat: a panel that lost power
// comes back dark until it gets this again.
bool configureOled() {
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    return false;
  }

  display.setRotation(settingsOledRotate180() ? 2 : 0);

  display.clearDisplay();
  display.display();
  return true;
}

}  // namespace

void initOled() {
  serialLogPrintln();
  serialLogPrintln("Checking OLED at 0x3C...");

  if (!i2cDeviceConnected(OLED_ADDRESS)) {
    serialLogPrintln("ERROR: OLED not found");
    oledAvailable = false;
    return;
  }

  serialLogPrintln("OLED found");

  if (!configureOled()) {
    serialLogPrintln("ERROR: OLED initialization failed");
    oledAvailable = false;
    return;
  }

  oledAvailable = true;

  serialLogPrintln("OLED initialized");
}

bool restartOled() {
  oledAvailable = i2cDeviceConnected(OLED_ADDRESS) && configureOled();
  return oledAvailable;
}
