#include "extensions/lcd1602/hw.h"

#include <Arduino.h>
#include <Wire.h>

#include "display/oled.h"
#include "pins.h"

namespace {

constexpr uint8_t kCmdPrefix = 0x80;
constexpr uint8_t kDataPrefix = 0x40;
constexpr uint8_t kClearDisplay = 0x01;
constexpr uint8_t kEntryModeSet = 0x04;
constexpr uint8_t kDisplayControl = 0x08;
constexpr uint8_t kFunctionSet = 0x20;
constexpr uint8_t kEntryLeft = 0x02;
constexpr uint8_t kDisplayOn = 0x04;
constexpr uint8_t kTwoLine = 0x08;

constexpr uint8_t kLedShutdownReg = 0x00;
constexpr uint8_t kLedModeReg = 0x02;
constexpr uint8_t kLedCurrentReg = 0x03;
constexpr uint8_t kLedPwm1Reg = 0x04;
constexpr uint8_t kLedPwmUpdateReg = 0x07;
constexpr uint8_t kLedControlReg = 0x1D;

void lcdWrite(uint8_t prefix, uint8_t value) {
  Wire.beginTransmission(LCD1602_ADDRESS);
  Wire.write(prefix);
  Wire.write(value);
  Wire.endTransmission();
}

void lcdCommand(uint8_t value) {
  lcdWrite(kCmdPrefix, value);
}

void ledWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LCD1602_LED_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

}  // namespace

bool lcd1602HwPresent() {
  return i2cDeviceConnected(LCD1602_ADDRESS);
}

bool lcd1602HwLedPresent() {
  return i2cDeviceConnected(LCD1602_LED_ADDRESS);
}

bool lcd1602HwInit() {
  if (!lcd1602HwPresent()) {
    return false;
  }

  delay(50);
  constexpr uint8_t function = kFunctionSet | kTwoLine;
  lcdCommand(function);
  delay(5);
  lcdCommand(function);
  delay(5);
  lcdCommand(function);
  lcdCommand(kDisplayControl | kDisplayOn);
  lcdCommand(kClearDisplay);
  delay(2);
  lcdCommand(kEntryModeSet | kEntryLeft);

  if (lcd1602HwLedPresent()) {
    ledWrite(kLedShutdownReg, 0x20);
    ledWrite(kLedModeReg, 0x00);
    ledWrite(kLedCurrentReg, 0x00);
    ledWrite(kLedControlReg, 0x01);
  }

  return true;
}

void lcd1602HwClear() {
  lcdCommand(kClearDisplay);
  delay(2);
}

void lcd1602HwShowLine(uint8_t row, const char* cols16) {
  const uint8_t addr = static_cast<uint8_t>((row == 0 ? 0x80 : 0xC0));
  lcdCommand(addr);
  for (uint8_t i = 0; i < 16; i++) {
    const char ch = cols16[i] != '\0' ? cols16[i] : ' ';
    lcdWrite(kDataPrefix, static_cast<uint8_t>(ch));
  }
}

bool lcd1602HwSetBrightness(uint8_t percent) {
  if (!lcd1602HwLedPresent()) {
    return false;
  }

  if (percent > 100) {
    percent = 100;
  }

  if (percent == 0) {
    ledWrite(kLedShutdownReg, 0x00);
    return true;
  }

  ledWrite(kLedShutdownReg, 0x20);
  ledWrite(kLedModeReg, 0x00);
  ledWrite(kLedPwm1Reg, static_cast<uint8_t>((percent * 255) / 100));
  ledWrite(kLedPwmUpdateReg, 0x00);
  ledWrite(kLedControlReg, 0x01);
  return true;
}
