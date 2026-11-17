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

constexpr size_t kOledFrameBytes = OLED_WIDTH * ((OLED_HEIGHT + 7) / 8);
// Frame bytes per transfer, well inside Wire's buffer.
constexpr size_t kOledFrameChunk = 32;
// The driver runs its own transfers at 400 kHz and drops back to 100 kHz for
// the PCA9685 (its clkDuring / clkAfter defaults). The checked transfers here
// do the same, or a frame would take four times as long.
constexpr uint32_t kOledClockHz = 400000;
constexpr uint32_t kBusClockHz = 100000;

// What begin() sends a 128x32 panel on its internal charge pump, minus the
// final DISPLAYON. The driver ignores failed writes, and a panel that missed
// part of this still answers its address, so this copy checks each transfer.
constexpr uint8_t kOledSetup[] = {
  SSD1306_DISPLAYOFF,
  SSD1306_SETDISPLAYCLOCKDIV, 0x80,
  SSD1306_SETMULTIPLEX, OLED_HEIGHT - 1,
  SSD1306_SETDISPLAYOFFSET, 0x00,
  SSD1306_SETSTARTLINE | 0x00,
  SSD1306_CHARGEPUMP, 0x14,
  SSD1306_MEMORYMODE, 0x00,
  SSD1306_SEGREMAP | 0x01,
  SSD1306_COMSCANDEC,
  SSD1306_SETCOMPINS, 0x02,
  SSD1306_SETCONTRAST, 0x8F,
  SSD1306_SETPRECHARGE, 0xF1,
  SSD1306_SETVCOMDETECT, 0x40,
  SSD1306_DISPLAYALLON_RESUME,
  SSD1306_NORMALDISPLAY,
  SSD1306_DEACTIVATE_SCROLL
};
static_assert(OLED_WIDTH == 128 && OLED_HEIGHT == 32, "kOledSetup is for a 128x32 panel");

constexpr uint8_t kOledFrameWindow[] = {
  SSD1306_PAGEADDR, 0x00, 0xFF,
  SSD1306_COLUMNADDR, 0x00, OLED_WIDTH - 1
};

// One I2C transfer: a control byte (0x00 commands, 0x40 frame data), then
// the bytes. True when the panel acknowledged all of it.
bool sendOled(uint8_t control, const uint8_t* bytes, size_t count) {
  Wire.beginTransmission(OLED_ADDRESS);
  Wire.write(control);
  Wire.write(bytes, count);
  return Wire.endTransmission() == 0;
}

bool sendOledSetupAndFrame(bool panelOn) {
  const uint8_t* frame = display.getBuffer();

  if (frame == nullptr ||
      !sendOled(0x00, kOledSetup, sizeof(kOledSetup)) ||
      !sendOled(0x00, kOledFrameWindow, sizeof(kOledFrameWindow))) {
    return false;
  }

  for (size_t offset = 0; offset < kOledFrameBytes; offset += kOledFrameChunk) {
    if (!sendOled(0x40, frame + offset, kOledFrameChunk)) {
      return false;
    }
  }

  if (!panelOn) {
    return true;
  }

  const uint8_t on = SSD1306_DISPLAYON;
  return sendOled(0x00, &on, 1);
}

// Sends the panel its full setup and the frame buffer, then turns it on when
// panelOn. Safe to repeat: a panel that lost power comes back dark until it
// gets this again. The frame goes over while the panel is still off, so it
// wakes showing it. Leaves the frame buffer and the rotation as they are.
bool configureOled(bool panelOn) {
  Wire.setClock(kOledClockHz);
  const bool ok = sendOledSetupAndFrame(panelOn);
  Wire.setClock(kBusClockHz);
  return ok;
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

  // begin() allocates and clears the frame buffer and sets the driver up;
  // configureOled() then sends the setup again with each transfer checked.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS) || !configureOled(true)) {
    serialLogPrintln("ERROR: OLED initialization failed");
    oledAvailable = false;
    return;
  }

  display.setRotation(settingsOledRotate180() ? 2 : 0);
  oledAvailable = true;

  serialLogPrintln("OLED initialized");
}

// Not begin(): it clears the frame buffer and ignores failed writes. The
// rotation is left alone too, since the setup wizard may be previewing one it
// has not saved. The frame buffer still holds the last frame, so a static
// screen comes back too.
bool restartOled(bool panelOn) {
  oledAvailable = i2cDeviceConnected(OLED_ADDRESS) && configureOled(panelOn);
  return oledAvailable;
}
