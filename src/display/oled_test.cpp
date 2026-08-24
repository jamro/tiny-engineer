#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/oled.h"
#include "display/oled_internal.h"
#include "serial_log.h"

void runOledTest() {
  if (!oledAvailable) {
    return;
  }

  serialLogPrintln();
  serialLogPrintln("==========================");
  serialLogPrintln("OLED TEST");
  serialLogPrintln("==========================");

  showOledText(
    "Tiny Engineer",
    "OLED test"
  );

  delay(1200);

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  display.setCursor(10, 8);
  display.print("HELLO");

  display.display();

  delay(1200);

  display.clearDisplay();

  display.drawRect(
    0,
    0,
    OLED_WIDTH,
    OLED_HEIGHT,
    SSD1306_WHITE
  );

  display.drawLine(
    0,
    0,
    OLED_WIDTH - 1,
    OLED_HEIGHT - 1,
    SSD1306_WHITE
  );

  display.drawLine(
    OLED_WIDTH - 1,
    0,
    0,
    OLED_HEIGHT - 1,
    SSD1306_WHITE
  );

  display.display();

  delay(1200);

  showOledText(
    "OLED: OK",
    "I2C 0x3C"
  );

  serialLogPrintln("OLED OK");

  delay(800);
}
