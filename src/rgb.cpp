#include <Arduino.h>
#include "esp32-hal-rgb-led.h"

#include "pins.h"
#include "rgb.h"

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_LED_PIN, r, g, b);
}

void runRgbTest() {
  Serial.println("RGB test");

  setRgb(64, 0, 0);
  delay(300);

  setRgb(0, 64, 0);
  delay(300);

  setRgb(0, 0, 64);
  delay(300);

  setRgb(32, 32, 32);
  delay(300);

  setRgb(0, 0, 0);

  Serial.println("RGB OK");
}
