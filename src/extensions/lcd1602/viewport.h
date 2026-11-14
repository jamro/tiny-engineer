#pragma once

#include <cstring>

#include "extensions/lcd1602/lcd1602.h"

// Pure 16-col viewport: short lines use align; long lines show [i..i+16) wrapping.
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
      out[i] = line.text[(scrollIndex + i) % len];
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
