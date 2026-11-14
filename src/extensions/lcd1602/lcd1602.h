#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t kLcd1602Cols = 16;
constexpr size_t kLcd1602LineMax = 64;

enum class Lcd1602Align : uint8_t {
  Left = 0,
  Right = 1,
  Center = 2,
};

struct Lcd1602LineState {
  char text[kLcd1602LineMax + 1] = {};
  Lcd1602Align align = Lcd1602Align::Left;
};

struct Lcd1602TextState {
  Lcd1602LineState line1;
  Lcd1602LineState line2;
};

bool lcd1602Enabled();
bool lcd1602HardwareOk();

void lcd1602ApplyFromSettings();
void lcd1602Poll();

const Lcd1602TextState& lcd1602TextState();
uint8_t lcd1602Brightness();

// Returns false when hardware is missing (caller should 503). Updates last on success.
bool lcd1602SetText(const Lcd1602LineState* line1, const Lcd1602LineState* line2);
bool lcd1602ClearText();
bool lcd1602SetBrightness(uint8_t percent);

Lcd1602Align lcd1602ParseAlign(const char* align, bool* ok);
const char* lcd1602AlignName(Lcd1602Align align);
