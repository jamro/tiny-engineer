#pragma once

#include <cstdint>

// Small helpers shared by the kawaii modules.

namespace kawaii {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 2.0f * kPi;

// Returns a value in [lo, hi). Firmware passes anim::randRangeMs; tests pass a
// seeded generator so behavior is reproducible.
using RandRange = uint32_t (*)(uint32_t lo, uint32_t hi);

inline float clamp01(float v) {
  if (v < 0.0f) {
    return 0.0f;
  }
  return v > 1.0f ? 1.0f : v;
}

inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

// True once `now` has passed `at`, across the millis() wrap.
inline bool reached(uint32_t now, uint32_t at) {
  return (int32_t)(now - at) >= 0;
}

}  // namespace kawaii
