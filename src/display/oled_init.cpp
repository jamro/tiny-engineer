#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "serial_log.h"

bool i2cDeviceConnected(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void initOled() {
  serialLogPrintln();
  serialLogPrintln("Checking OLED at 0x3C...");

  if (!i2cDeviceConnected(OLED_ADDRESS)) {
    serialLogPrintln("ERROR: OLED not found");
    oledAvailable = false;
    return;
  }

  serialLogPrintln("OLED found");

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    serialLogPrintln("ERROR: OLED initialization failed");
    oledAvailable = false;
    return;
  }

  // Rotate OLED by 180 degrees
  display.setRotation(2);

  display.clearDisplay();
  display.display();

  oledAvailable = true;

  serialLogPrintln("OLED initialized");
}
