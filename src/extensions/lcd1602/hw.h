#pragma once

#include <cstdint>

enum class Lcd1602LedKind : uint8_t {
  None = 0,
  Sn3193 = 1,
  Pca9633 = 2,
};

bool lcd1602HwPresent();
bool lcd1602HwLedPresent();
Lcd1602LedKind lcd1602HwLedKind();
bool lcd1602HwInit();
void lcd1602HwResetLed();
void lcd1602HwClear();
void lcd1602HwShowLine(uint8_t row, const char* cols16);
// SN3193 only. For PCA9633 use lcd1602HwSetRgbPwm with pre-scaled channels.
bool lcd1602HwSetBrightness(uint8_t percent);
bool lcd1602HwSetRgbPwm(uint8_t r, uint8_t g, uint8_t b);
