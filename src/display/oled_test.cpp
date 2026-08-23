#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "display/oled.h"
#include "display/oled_internal.h"

void runOledTest() {
  if (!oledAvailable) {
    return;
  }

  Serial.println();
  Serial.println("==========================");
  Serial.println("OLED TEST");
  Serial.println("==========================");

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

  Serial.println("OLED OK");

  delay(800);
}
