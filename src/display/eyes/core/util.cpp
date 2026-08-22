#include "display/eyes/core/util.h"

#include "display/eyes/core/constants.h"

namespace eyes {

Eye eyeWithHeight(int16_t x, int16_t width, int16_t height) {
  return {
    x,
    (int16_t)(EYE_CENTER_Y - height / 2),
    width,
    height
  };
}

Eye renderEye(const Eye& base, float openAmount) {
  const int16_t renderHeight =
    (int16_t)(base.height * openAmount);

  if (renderHeight <= 0) {
    return {base.x, base.y, base.width, 0};
  }

  return {
    base.x,
    (int16_t)(base.y + (base.height - renderHeight) / 2),
    base.width,
    renderHeight
  };
}

float phaseProgress(
  uint32_t now,
  uint32_t phaseStartedMs,
  uint32_t durationMs
) {
  if (durationMs == 0) {
    return 1.0f;
  }

  const uint32_t elapsed = now - phaseStartedMs;

  if (elapsed >= durationMs) {
    return 1.0f;
  }

  return (float)elapsed / (float)durationMs;
}

float moveProgress(uint32_t now, uint32_t startMs, uint32_t durationMs) {
  if (durationMs == 0) {
    return 1.0f;
  }

  if (now < startMs) {
    return 0.0f;
  }

  const uint32_t elapsed = now - startMs;

  if (elapsed >= durationMs) {
    return 1.0f;
  }

  return (float)elapsed / (float)durationMs;
}

int16_t lerpInt(int16_t from, int16_t to, float t) {
  return (int16_t)(from + (to - from) * t);
}

}  // namespace eyes
