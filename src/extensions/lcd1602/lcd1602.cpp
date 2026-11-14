#include "extensions/lcd1602/lcd1602.h"

#include <Arduino.h>
#include <cstring>

#include "extensions/lcd1602/hw.h"
#include "extensions/lcd1602/viewport.h"
#include "serial_log.h"
#include "settings/nvs.h"
#include "settings/settings.h"

namespace {

constexpr uint32_t kScrollStepMs = 375;
constexpr uint32_t kScrollWrapPauseMs = 750;
constexpr uint8_t kAlignLeft = 0;
constexpr uint8_t kAlignRight = 1;
constexpr uint8_t kAlignCenter = 2;

#pragma pack(push, 1)
struct Lcd1602LastBlob {
  char line1[kLcd1602LineMax + 1];
  char line2[kLcd1602LineMax + 1];
  uint8_t align1;
  uint8_t align2;
  uint8_t brightness;
  uint8_t reserved;
};
#pragma pack(pop)

bool g_active = false;
bool g_hwOk = false;
Lcd1602TextState g_text{};
uint8_t g_brightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
size_t g_scrollIndex[2] = {0, 0};
uint32_t g_nextScrollMs[2] = {0, 0};
bool g_atWrapPause[2] = {false, false};

Lcd1602Align alignFromByte(uint8_t value) {
  switch (value) {
    case kAlignRight:
      return Lcd1602Align::Right;
    case kAlignCenter:
      return Lcd1602Align::Center;
    default:
      return Lcd1602Align::Left;
  }
}

uint8_t alignToByte(Lcd1602Align align) {
  switch (align) {
    case Lcd1602Align::Right:
      return kAlignRight;
    case Lcd1602Align::Center:
      return kAlignCenter;
    case Lcd1602Align::Left:
    default:
      return kAlignLeft;
  }
}

void renderLine(uint8_t row) {
  if (!g_hwOk) {
    return;
  }

  char cols[kLcd1602Cols + 1];
  const Lcd1602LineState& line = row == 0 ? g_text.line1 : g_text.line2;
  lcd1602FormatViewport(line, g_scrollIndex[row], cols);
  lcd1602HwShowLine(row, cols);
}

void renderAll() {
  renderLine(0);
  renderLine(1);
}

void resetScroll(uint8_t row) {
  g_scrollIndex[row] = 0;
  g_atWrapPause[row] = false;
  g_nextScrollMs[row] = millis() + kScrollStepMs;
}

bool lineNeedsScroll(const Lcd1602LineState& line) {
  return strlen(line.text) > kLcd1602Cols;
}

void persistLast() {
  Lcd1602LastBlob blob{};
  strlcpy(blob.line1, g_text.line1.text, sizeof(blob.line1));
  strlcpy(blob.line2, g_text.line2.text, sizeof(blob.line2));
  blob.align1 = alignToByte(g_text.line1.align);
  blob.align2 = alignToByte(g_text.line2.align);
  blob.brightness = g_brightness;

  if (!g_settingsPrefs.begin(kNs, false)) {
    serialLogPrintln("LCD1602: last NVS open failed");
    return;
  }

  const bool ok =
    g_settingsPrefs.putBytes(kKeyLcd1602Last, &blob, sizeof(blob)) == sizeof(blob);
  g_settingsPrefs.end();

  if (!ok) {
    serialLogPrintln("LCD1602: last NVS write failed");
  }
}

bool loadLast(Lcd1602TextState& text, uint8_t& brightness) {
  if (!g_settingsPrefs.begin(kNs, true)) {
    return false;
  }

  Lcd1602LastBlob blob{};
  const size_t got = g_settingsPrefs.isKey(kKeyLcd1602Last)
    ? g_settingsPrefs.getBytes(kKeyLcd1602Last, &blob, sizeof(blob))
    : 0;
  g_settingsPrefs.end();

  if (got != sizeof(blob)) {
    return false;
  }

  blob.line1[sizeof(blob.line1) - 1] = '\0';
  blob.line2[sizeof(blob.line2) - 1] = '\0';
  strlcpy(text.line1.text, blob.line1, sizeof(text.line1.text));
  strlcpy(text.line2.text, blob.line2, sizeof(text.line2.text));
  text.line1.align = alignFromByte(blob.align1);
  text.line2.align = alignFromByte(blob.align2);
  brightness = blob.brightness > 100 ? SETTINGS_DEFAULT_LCD1602_BRIGHTNESS : blob.brightness;
  return true;
}

void applyBrightnessPolicy(bool fromLast, uint8_t lastBrightness) {
  if (fromLast) {
    g_brightness = lastBrightness;
    if (g_hwOk) {
      lcd1602HwSetBrightness(g_brightness);
    }
    return;
  }

  if (settingsLcd1602BacklightBoot()) {
    g_brightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
    if (g_hwOk) {
      lcd1602HwSetBrightness(g_brightness);
    }
  } else if (g_hwOk) {
    g_brightness = 0;
    lcd1602HwSetBrightness(0);
  }
}

void applyBootContent() {
  const char* mode = settingsLcd1602Boot();
  Lcd1602TextState next{};
  uint8_t lastBrightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
  bool haveLast = false;

  if (strcmp(mode, "last") == 0) {
    haveLast = loadLast(next, lastBrightness);
  } else if (strcmp(mode, "config") == 0) {
    strlcpy(next.line1.text, settingsLcd1602Line1(), sizeof(next.line1.text));
    strlcpy(next.line2.text, settingsLcd1602Line2(), sizeof(next.line2.text));
  }

  g_text = next;
  resetScroll(0);
  resetScroll(1);

  if (g_hwOk) {
    lcd1602HwClear();
    renderAll();
  }

  applyBrightnessPolicy(haveLast && strcmp(mode, "last") == 0, lastBrightness);
}

}  // namespace

bool lcd1602Enabled() {
  return g_active;
}

bool lcd1602HardwareOk() {
  return g_hwOk;
}

void lcd1602ApplyFromSettings() {
  if (!settingsLcd1602()) {
    g_active = false;
    g_hwOk = false;
    serialLogPrintln("LCD1602 extension off");
    return;
  }

  g_active = true;
  g_hwOk = lcd1602HwInit();

  if (!g_hwOk) {
    serialLogPrintln("LCD1602 not found at 0x3E");
  } else {
    serialLogPrintln("LCD1602 initialized");
  }

  applyBootContent();
}

void lcd1602Poll() {
  if (!g_active || !g_hwOk) {
    return;
  }

  const uint32_t now = millis();
  for (uint8_t row = 0; row < 2; row++) {
    const Lcd1602LineState& line = row == 0 ? g_text.line1 : g_text.line2;
    if (!lineNeedsScroll(line)) {
      continue;
    }

    if (static_cast<int32_t>(now - g_nextScrollMs[row]) < 0) {
      continue;
    }

    const size_t len = strlen(line.text);
    g_scrollIndex[row] = (g_scrollIndex[row] + 1) % len;
    if (g_scrollIndex[row] == 0) {
      g_atWrapPause[row] = true;
      g_nextScrollMs[row] = now + kScrollWrapPauseMs;
    } else {
      g_atWrapPause[row] = false;
      g_nextScrollMs[row] = now + kScrollStepMs;
    }
    renderLine(row);
  }
}

const Lcd1602TextState& lcd1602TextState() {
  return g_text;
}

uint8_t lcd1602Brightness() {
  return g_brightness;
}

bool lcd1602SetText(const Lcd1602LineState* line1, const Lcd1602LineState* line2) {
  if (!g_active) {
    return false;
  }

  if (!g_hwOk && !lcd1602HwPresent()) {
    return false;
  }

  if (!g_hwOk) {
    g_hwOk = lcd1602HwInit();
    if (!g_hwOk) {
      return false;
    }
  }

  if (line1 != nullptr) {
    g_text.line1 = *line1;
    resetScroll(0);
  }

  if (line2 != nullptr) {
    g_text.line2 = *line2;
    resetScroll(1);
  }

  renderAll();
  persistLast();
  return true;
}

bool lcd1602ClearText() {
  Lcd1602LineState empty{};
  return lcd1602SetText(&empty, &empty);
}

bool lcd1602SetBrightness(uint8_t percent) {
  if (!g_active) {
    return false;
  }

  if (percent > 100) {
    return false;
  }

  if (!lcd1602HwSetBrightness(percent)) {
    return false;
  }

  g_brightness = percent;
  persistLast();
  return true;
}

Lcd1602Align lcd1602ParseAlign(const char* align, bool* ok) {
  if (ok != nullptr) {
    *ok = true;
  }

  if (align == nullptr || align[0] == '\0' || strcmp(align, "left") == 0) {
    return Lcd1602Align::Left;
  }

  if (strcmp(align, "right") == 0) {
    return Lcd1602Align::Right;
  }

  if (strcmp(align, "center") == 0) {
    return Lcd1602Align::Center;
  }

  if (ok != nullptr) {
    *ok = false;
  }
  return Lcd1602Align::Left;
}

const char* lcd1602AlignName(Lcd1602Align align) {
  switch (align) {
    case Lcd1602Align::Right:
      return "right";
    case Lcd1602Align::Center:
      return "center";
    case Lcd1602Align::Left:
    default:
      return "left";
  }
}
