#pragma once

#include <cstdint>

bool lcd1602HwPresent();
bool lcd1602HwLedPresent();
bool lcd1602HwInit();
void lcd1602HwResetLed();
void lcd1602HwClear();
void lcd1602HwShowLine(uint8_t row, const char* cols16);
bool lcd1602HwSetBrightness(uint8_t percent);
