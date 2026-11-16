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

// SN3193 (Blue / Gray / Green modules)
constexpr uint8_t kLedShutdownReg = 0x00;
constexpr uint8_t kLedModeReg = 0x02;
constexpr uint8_t kLedCurrentReg = 0x03;
constexpr uint8_t kLedPwm1Reg = 0x04;
constexpr uint8_t kLedPwmUpdateReg = 0x07;
constexpr uint8_t kLedControlReg = 0x1D;

// PCA9633 (RGB module) — Waveshare: PWM0=blue, PWM1=green, PWM2=red
constexpr uint8_t kRgbMode1Reg = 0x00;
constexpr uint8_t kRgbMode2Reg = 0x01;
constexpr uint8_t kRgbPwm0Reg = 0x02;
constexpr uint8_t kRgbPwm1Reg = 0x03;
constexpr uint8_t kRgbPwm2Reg = 0x04;
constexpr uint8_t kRgbLedoutReg = 0x08;

Lcd1602LedKind g_ledChip = Lcd1602LedKind::None;

void lcdWrite(uint8_t prefix, uint8_t value) {
  Wire.beginTransmission(LCD1602_ADDRESS);
  Wire.write(prefix);
  Wire.write(value);
  Wire.endTransmission();
}

void lcdCommand(uint8_t value) {
  lcdWrite(kCmdPrefix, value);
}

void sn3193Write(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LCD1602_LED_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void pca9633Write(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LCD1602_RGB_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool sn3193Present() {
  return i2cDeviceConnected(LCD1602_LED_ADDRESS);
}

bool pca9633Present() {
  return i2cDeviceConnected(LCD1602_RGB_ADDRESS);
}

Lcd1602LedKind detectLedChip() {
  if (sn3193Present()) {
    return Lcd1602LedKind::Sn3193;
  }
  if (pca9633Present()) {
    return Lcd1602LedKind::Pca9633;
  }
  return Lcd1602LedKind::None;
}

void initSn3193() {
  sn3193Write(kLedShutdownReg, 0x20);
  sn3193Write(kLedModeReg, 0x00);
  sn3193Write(kLedCurrentReg, 0x00);
  sn3193Write(kLedControlReg, 0x01);
}

void initPca9633() {
  pca9633Write(kRgbMode1Reg, 0x00);
  pca9633Write(kRgbMode2Reg, 0x20);
  pca9633Write(kRgbLedoutReg, 0xFF);
}

bool setBrightnessSn3193(uint8_t percent) {
  if (percent == 0) {
    sn3193Write(kLedShutdownReg, 0x00);
    return true;
  }

  sn3193Write(kLedShutdownReg, 0x20);
  sn3193Write(kLedModeReg, 0x00);
  sn3193Write(kLedPwm1Reg, static_cast<uint8_t>((percent * 255) / 100));
  sn3193Write(kLedPwmUpdateReg, 0x00);
  sn3193Write(kLedControlReg, 0x01);
  return true;
}

Lcd1602LedKind ensureLedChip() {
  if (g_ledChip != Lcd1602LedKind::None) {
    return g_ledChip;
  }
  g_ledChip = detectLedChip();
  if (g_ledChip == Lcd1602LedKind::Sn3193) {
    initSn3193();
  } else if (g_ledChip == Lcd1602LedKind::Pca9633) {
    initPca9633();
  }
  return g_ledChip;
}

}  // namespace

void lcd1602HwResetLed() {
  g_ledChip = Lcd1602LedKind::None;
}

bool lcd1602HwPresent() {
  return i2cDeviceConnected(LCD1602_ADDRESS);
}

bool lcd1602HwLedPresent() {
  if (g_ledChip != Lcd1602LedKind::None) {
    return true;
  }
  return sn3193Present() || pca9633Present();
}

Lcd1602LedKind lcd1602HwLedKind() {
  return ensureLedChip();
}

bool lcd1602HwInit() {
  lcd1602HwResetLed();

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

  ensureLedChip();
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
  if (percent > 100) {
    percent = 100;
  }

  const Lcd1602LedKind chip = ensureLedChip();
  if (chip != Lcd1602LedKind::Sn3193) {
    return false;
  }
  return setBrightnessSn3193(percent);
}

bool lcd1602HwSetRgbPwm(uint8_t r, uint8_t g, uint8_t b) {
  const Lcd1602LedKind chip = ensureLedChip();
  if (chip != Lcd1602LedKind::Pca9633) {
    return false;
  }

  // Waveshare: PWM0=blue, PWM1=green, PWM2=red
  pca9633Write(kRgbPwm0Reg, b);
  pca9633Write(kRgbPwm1Reg, g);
  pca9633Write(kRgbPwm2Reg, r);
  return true;
}
