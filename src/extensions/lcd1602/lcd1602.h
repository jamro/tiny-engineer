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

struct Lcd1602Color {
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
};

bool lcd1602Enabled();
bool lcd1602HardwareOk();

void lcd1602ApplyFromSettings();
void lcd1602Poll();
// Smooth brightness change (sleep→0, wake→100). No-op if extension off. Duration 0 snaps.
void lcd1602FadeBrightnessTo(uint8_t targetPercent, uint32_t durationMs);

// Opt-in HELLO/HUMAN overlay for every Welcome animation. No-op if off/disabled.
void lcd1602BeginWelcomeHello();
void lcd1602EndWelcomeHello();

const Lcd1602TextState& lcd1602TextState();
uint8_t lcd1602Brightness();
Lcd1602Color lcd1602Color();

// Returns false when hardware is missing (caller should 503). Updates last on success.
bool lcd1602SetText(const Lcd1602LineState* line1, const Lcd1602LineState* line2);
bool lcd1602ClearText();
bool lcd1602SetBrightness(uint8_t percent);
// Sets base RGB and applies PWM scaled by brightness. Caller maps chip errors via lcd1602HwLedKind.
bool lcd1602SetColor(uint8_t r, uint8_t g, uint8_t b);

Lcd1602Align lcd1602ParseAlign(const char* align, bool* ok);
const char* lcd1602AlignName(Lcd1602Align align);
