#pragma once

#include <cstdint>

// Small helpers shared by the kawaii modules.

namespace kawaii {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 2.0f * kPi;

// Returns a value in [lo, hi): hi itself is never returned, and an empty
// range (hi <= lo) returns lo. Tests pass a seeded generator so behavior is
// reproducible; firmware adapts anim::randRangeMs with upperExclusive.
using RandRange = uint32_t (*)(uint32_t lo, uint32_t hi);

// Adapts a generator that returns [lo, hi] (inclusive, like
// anim::randRangeMs) to RandRange's [lo, hi).
template <uint32_t (*Inclusive)(uint32_t lo, uint32_t hi)>
uint32_t upperExclusive(uint32_t lo, uint32_t hi) {
  return hi <= lo ? lo : Inclusive(lo, hi - 1u);
}

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
