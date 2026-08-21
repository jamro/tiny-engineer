#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "oled.h"

Adafruit_SSD1306 display(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  -1
);

bool oledAvailable = false;

bool i2cDeviceConnected(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
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

void initOled() {
  Serial.println();
  Serial.println("Checking OLED at 0x3C...");

  if (!i2cDeviceConnected(OLED_ADDRESS)) {
    Serial.println("ERROR: OLED not found");
    oledAvailable = false;
    return;
  }

  Serial.println("OLED found");

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    Serial.println("ERROR: OLED initialization failed");
    oledAvailable = false;
    return;
  }

  // Rotate OLED by 180 degrees
  display.setRotation(2);

  display.clearDisplay();
  display.display();

  oledAvailable = true;

  Serial.println("OLED initialized");
}

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

void showServoProgress(
  int step,
  int totalSteps,
  const char* action
) {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("SERVO TEST x5");

  display.setCursor(0, 11);

  display.print(step);
  display.print("/");
  display.print(totalSteps);
  display.print(" ");

  display.println(action);

  constexpr int barX = 0;
  constexpr int barY = 23;
  constexpr int barWidth = 128;
  constexpr int barHeight = 9;

  display.drawRect(
    barX,
    barY,
    barWidth,
    barHeight,
    SSD1306_WHITE
  );

  const int progressWidth =
    (barWidth - 2) * step / totalSteps;

  display.fillRect(
    barX + 1,
    barY + 1,
    progressWidth,
    barHeight - 2,
    SSD1306_WHITE
  );

  display.display();
}

void showServoTestFinished() {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 2);
  display.println("5 SERVOS");

  display.setTextSize(2);
  display.setCursor(42, 14);
  display.println("OK");

  display.display();
}
