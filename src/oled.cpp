#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "eyes.h"
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

void showIdleScreen() {
  startEyes();
}

namespace {

void fillRoundedEye(
  const Eye& eye,
  int cornerRadius
) {
  if (eye.width <= 0 || eye.height <= 0) {
    return;
  }

  const int radius = min(
    cornerRadius,
    min(eye.width / 2, eye.height / 2)
  );

  if (radius <= 0) {
    display.fillRect(
      eye.x,
      eye.y,
      eye.width,
      eye.height,
      SSD1306_WHITE
    );
    return;
  }

  display.fillRect(
    eye.x + radius,
    eye.y,
    eye.width - 2 * radius,
    eye.height,
    SSD1306_WHITE
  );

  display.fillRect(
    eye.x,
    eye.y + radius,
    eye.width,
    eye.height - 2 * radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + radius,
    eye.y + radius,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + eye.width - radius - 1,
    eye.y + radius,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + radius,
    eye.y + eye.height - radius - 1,
    radius,
    SSD1306_WHITE
  );

  display.fillCircle(
    eye.x + eye.width - radius - 1,
    eye.y + eye.height - radius - 1,
    radius,
    SSD1306_WHITE
  );
}

}  // namespace

void drawEyes(
  const Eye& left,
  const Eye& right,
  int cornerRadius
) {
  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();
  fillRoundedEye(left, cornerRadius);
  fillRoundedEye(right, cornerRadius);
  display.display();
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
