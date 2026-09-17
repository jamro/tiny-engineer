#pragma once

#include <cstddef>
#include <cstdint>

#if defined(ARDUINO)
#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif
#endif

#ifndef TE_EXPR_PROGMEM
#if defined(ARDUINO)
#define TE_EXPR_PROGMEM PROGMEM
#else
#define TE_EXPR_PROGMEM
#endif
#endif

namespace tiny_engineer {
namespace expressions {

enum class Expression : uint8_t {
  Idle,
  Happy,
  Laugh,
  Wink,
  Curious,
  Thinking,
  Surprise,
  Smug,
  Sleepy,
  Sleep,
  Sad,
  Cry,
  Angry,
  Panic,
  Shy,
  Love
};

constexpr std::size_t kExpressionCount = 16;
constexpr std::size_t kWidth = 128;
constexpr std::size_t kHeight = 32;
constexpr std::size_t kFrameBytes = 512;
constexpr std::size_t kFramesPerExpression = 30;
constexpr uint32_t kFrameDurationMs = 100;
constexpr uint32_t kLoopDurationMs = 3000;

// Stable lowercase identifiers for the 16 selectable expressions; invalid
// enum values return nullptr.
const char* name(Expression expression);

// Render the frame at elapsedMs since the caller selected this expression.
// The caller owns timing, selection, and display output. Animation loops every
// kLoopDurationMs; this function never reads a clock or accesses a display.
// Output is 128x32, row-major, MSB first, compatible with drawBitmap().
// Invalid arguments or malformed assets return false without changing output.
// A valid call writes exactly kFrameBytes and leaves any remaining bytes intact.
bool render(Expression expression, uint32_t elapsedMs,
            uint8_t* output, std::size_t outputSize);

}  // namespace expressions
}  // namespace tiny_engineer
