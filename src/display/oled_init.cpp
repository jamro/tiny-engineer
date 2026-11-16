#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <cstring>

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

constexpr size_t kOledFrameBytes = OLED_WIDTH * ((OLED_HEIGHT + 7) / 8);

// The frame on the panel before a restart. begin() clears the frame buffer,
// and static screens are never drawn again, so a restart puts this back.
uint8_t g_savedFrame[kOledFrameBytes];
// Set from the first attempt of a restart until one succeeds: a failed
// attempt's begin() has already cleared the buffer, so later attempts must not
// save it again.
bool g_frameSaved = false;

// Sends the SSD1306 its full setup and turns the panel on. Safe to repeat: a
// panel that lost power comes back dark until it gets this again. Clears the
// frame buffer.
bool configureOled() {
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    return false;
  }

  display.setRotation(settingsOledRotate180() ? 2 : 0);
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

  display.display();
  oledAvailable = true;

  serialLogPrintln("OLED initialized");
}

bool restartOled(bool panelOn) {
  uint8_t* buffer = display.getBuffer();

  if (buffer != nullptr && !g_frameSaved) {
    memcpy(g_savedFrame, buffer, kOledFrameBytes);
    g_frameSaved = true;
  }

  oledAvailable = i2cDeviceConnected(OLED_ADDRESS) && configureOled();

  if (!oledAvailable) {
    return false;
  }

  // Dark first: the panel takes the frame while off, and wakes showing it.
  if (!panelOn) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  if (g_frameSaved) {
    memcpy(display.getBuffer(), g_savedFrame, kOledFrameBytes);
    g_frameSaved = false;
  }

  display.display();
  return true;
}
