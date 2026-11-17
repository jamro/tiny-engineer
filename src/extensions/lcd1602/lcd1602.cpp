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
struct Lcd1602LastBlobV1 {
  char line1[kLcd1602LineMax + 1];
  char line2[kLcd1602LineMax + 1];
  uint8_t align1;
  uint8_t align2;
  uint8_t brightness;
  uint8_t reserved;
};

struct Lcd1602LastBlob {
  char line1[kLcd1602LineMax + 1];
  char line2[kLcd1602LineMax + 1];
  uint8_t align1;
  uint8_t align2;
  uint8_t brightness;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
#pragma pack(pop)

static_assert(sizeof(Lcd1602LastBlobV1) == sizeof(Lcd1602LastBlob) - 2, "v1 blob size");

bool g_active = false;
bool g_hwOk = false;
Lcd1602TextState g_text{};
uint8_t g_brightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
Lcd1602Color g_color{};
size_t g_scrollIndex[2] = {0, 0};
uint32_t g_nextScrollMs[2] = {0, 0};
bool g_atWrapPause[2] = {false, false};

bool g_fadeActive = false;
uint32_t g_fadeStartMs = 0;
uint32_t g_fadeDurationMs = 0;
uint8_t g_fadeFrom = 0;
uint8_t g_fadeTo = 0;

float easeInOut(float t) {
  return t * t * (3.0f - 2.0f * t);
}

void clearBrightnessFade() {
  g_fadeActive = false;
}

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

uint8_t scaleChannel(uint8_t channel, uint8_t brightness) {
  return static_cast<uint8_t>((static_cast<uint16_t>(channel) * brightness) / 100);
}

bool applyBacklightHw() {
  const Lcd1602LedKind kind = lcd1602HwLedKind();
  if (kind == Lcd1602LedKind::None) {
    return false;
  }
  if (kind == Lcd1602LedKind::Sn3193) {
    return lcd1602HwSetBrightness(g_brightness);
  }
  return lcd1602HwSetRgbPwm(
    scaleChannel(g_color.r, g_brightness),
    scaleChannel(g_color.g, g_brightness),
    scaleChannel(g_color.b, g_brightness)
  );
}

void persistLast() {
  Lcd1602LastBlob blob{};
  strlcpy(blob.line1, g_text.line1.text, sizeof(blob.line1));
  strlcpy(blob.line2, g_text.line2.text, sizeof(blob.line2));
  blob.align1 = alignToByte(g_text.line1.align);
  blob.align2 = alignToByte(g_text.line2.align);
  blob.brightness = g_brightness;
  blob.r = g_color.r;
  blob.g = g_color.g;
  blob.b = g_color.b;

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

bool loadLast(Lcd1602TextState& text, uint8_t& brightness, Lcd1602Color& color) {
  if (!g_settingsPrefs.begin(kNs, true)) {
    return false;
  }

  Lcd1602LastBlob blob{};
  const size_t got = g_settingsPrefs.isKey(kKeyLcd1602Last)
    ? g_settingsPrefs.getBytes(kKeyLcd1602Last, &blob, sizeof(blob))
    : 0;
  g_settingsPrefs.end();

  if (got != sizeof(Lcd1602LastBlob) && got != sizeof(Lcd1602LastBlobV1)) {
    return false;
  }

  blob.line1[sizeof(blob.line1) - 1] = '\0';
  blob.line2[sizeof(blob.line2) - 1] = '\0';
  strlcpy(text.line1.text, blob.line1, sizeof(text.line1.text));
  strlcpy(text.line2.text, blob.line2, sizeof(text.line2.text));
  text.line1.align = alignFromByte(blob.align1);
  text.line2.align = alignFromByte(blob.align2);
  brightness = blob.brightness > 100 ? SETTINGS_DEFAULT_LCD1602_BRIGHTNESS : blob.brightness;

  if (got == sizeof(Lcd1602LastBlob)) {
    color.r = blob.r;
    color.g = blob.g;
    color.b = blob.b;
  } else {
    color = Lcd1602Color{};
  }
  return true;
}

void applyBrightnessPolicy(bool fromLast, uint8_t lastBrightness, const Lcd1602Color& lastColor) {
  if (fromLast) {
    g_brightness = lastBrightness;
    g_color = lastColor;
  } else {
    g_color = Lcd1602Color{};
    g_brightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
  }

  // Follow-sleep off means the panel stays lit while awake. Sleep may have
  // persisted brightness 0 into "last"; do not leave the backlight off on
  // enable/boot.
  if (!settingsLcd1602BacklightSleep() && g_brightness == 0) {
    g_brightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
  }

  if (g_hwOk) {
    applyBacklightHw();
  }
}

void applyBootContent() {
  const char* mode = settingsLcd1602Boot();
  Lcd1602TextState next{};
  uint8_t lastBrightness = SETTINGS_DEFAULT_LCD1602_BRIGHTNESS;
  Lcd1602Color lastColor{};
  bool haveLast = false;

  if (strcmp(mode, "last") == 0) {
    haveLast = loadLast(next, lastBrightness, lastColor);
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

  applyBrightnessPolicy(haveLast && strcmp(mode, "last") == 0, lastBrightness, lastColor);
}

}  // namespace

bool lcd1602Enabled() {
  return g_active;
}

bool lcd1602HardwareOk() {
  return g_hwOk;
}

void lcd1602FadeBrightnessTo(uint8_t targetPercent, uint32_t durationMs) {
  if (!g_active) {
    return;
  }

  if (targetPercent > 100) {
    targetPercent = 100;
  }

  if (durationMs == 0 || targetPercent == g_brightness) {
    clearBrightnessFade();
    g_brightness = targetPercent;
    if (g_hwOk) {
      applyBacklightHw();
    }
    persistLast();
    return;
  }

  g_fadeFrom = g_brightness;
  g_fadeTo = targetPercent;
  g_fadeStartMs = millis();
  g_fadeDurationMs = durationMs;
  g_fadeActive = true;
}

void lcd1602ApplyFromSettings() {
  if (!settingsLcd1602()) {
    g_active = false;
    g_hwOk = false;
    clearBrightnessFade();
    lcd1602HwResetLed();
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

  // Drop any in-flight sleep/wake fade so boot policy is not overwritten.
  clearBrightnessFade();
  applyBootContent();
}

void lcd1602Poll() {
  if (!g_active) {
    return;
  }

  const uint32_t now = millis();

  if (g_fadeActive && g_hwOk) {
    const uint32_t elapsed = now - g_fadeStartMs;
    if (elapsed >= g_fadeDurationMs) {
      g_brightness = g_fadeTo;
      clearBrightnessFade();
      applyBacklightHw();
      persistLast();
    } else {
      const float t = easeInOut(
        static_cast<float>(elapsed) / static_cast<float>(g_fadeDurationMs)
      );
      const float from = static_cast<float>(g_fadeFrom);
      const float to = static_cast<float>(g_fadeTo);
      g_brightness = static_cast<uint8_t>(from + (to - from) * t + 0.5f);
      applyBacklightHw();
    }
  }

  if (!g_hwOk) {
    return;
  }

  for (uint8_t row = 0; row < 2; row++) {
    const Lcd1602LineState& line = row == 0 ? g_text.line1 : g_text.line2;
    if (!lineNeedsScroll(line)) {
      continue;
    }

    if (static_cast<int32_t>(now - g_nextScrollMs[row]) < 0) {
      continue;
    }

    const size_t len = strlen(line.text);
    const size_t cycle = lcd1602ScrollCycleLen(len);
    g_scrollIndex[row] = (g_scrollIndex[row] + 1) % cycle;
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

Lcd1602Color lcd1602Color() {
  return g_color;
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

  clearBrightnessFade();
  g_brightness = percent;
  if (!applyBacklightHw()) {
    return false;
  }

  persistLast();
  return true;
}

bool lcd1602SetColor(uint8_t r, uint8_t g, uint8_t b) {
  if (!g_active) {
    return false;
  }

  if (lcd1602HwLedKind() != Lcd1602LedKind::Pca9633) {
    return false;
  }

  clearBrightnessFade();
  g_color.r = r;
  g_color.g = g;
  g_color.b = b;
  if (!applyBacklightHw()) {
    return false;
  }

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
