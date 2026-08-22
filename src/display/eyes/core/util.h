#pragma once

#include <stdint.h>

#include "display/eyes.h"

namespace eyes {

Eye eyeWithHeight(int16_t x, int16_t width, int16_t height);
Eye renderEye(const Eye& base, float openAmount);
float phaseProgress(
  uint32_t now,
  uint32_t phaseStartedMs,
  uint32_t durationMs
);
float moveProgress(uint32_t now, uint32_t startMs, uint32_t durationMs);
int16_t lerpInt(int16_t from, int16_t to, float t);

}  // namespace eyes
