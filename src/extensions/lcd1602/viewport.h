#pragma once

#include <cstring>

#include "extensions/lcd1602/lcd1602.h"

// Spaces inserted between end and start of a long-line marquee cycle.
constexpr size_t kLcd1602ScrollGap = 3;

inline size_t lcd1602ScrollCycleLen(size_t textLen) {
  return textLen + kLcd1602ScrollGap;
}

inline char lcd1602ScrollCharAt(const char* text, size_t textLen, size_t index) {
  const size_t cycle = lcd1602ScrollCycleLen(textLen);
  const size_t pos = index % cycle;
  if (pos >= textLen) {
    return ' ';
  }
  return text[pos];
}

// Pure 16-col viewport: short lines use align; long lines show [i..i+16) wrapping
// through text plus kLcd1602ScrollGap spaces so end/start do not glue together.
inline void lcd1602FormatViewport(
  const Lcd1602LineState& line,
  size_t scrollIndex,
  char out[kLcd1602Cols + 1]
) {
  const size_t len = strlen(line.text);
  memset(out, ' ', kLcd1602Cols);
  out[kLcd1602Cols] = '\0';

  if (len == 0) {
    return;
  }

  if (len > kLcd1602Cols) {
    for (size_t i = 0; i < kLcd1602Cols; i++) {
      out[i] = lcd1602ScrollCharAt(line.text, len, scrollIndex + i);
    }
    return;
  }

  size_t start = 0;
  if (line.align == Lcd1602Align::Right) {
    start = kLcd1602Cols - len;
  } else if (line.align == Lcd1602Align::Center) {
    start = (kLcd1602Cols - len) / 2;
  }

  memcpy(out + start, line.text, len);
}
