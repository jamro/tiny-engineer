#include "TinyEngineerExpressions.h"
#include "expression_assets.h"

#include <cstring>

namespace tiny_engineer {
namespace expressions {

namespace {

uint8_t readByte(const uint8_t* address) {
#if defined(ARDUINO)
  return pgm_read_byte(address);
#else
  return *address;
#endif
}

uint16_t readWord(const uint16_t* address) {
#if defined(ARDUINO)
  return pgm_read_word(address);
#else
  return *address;
#endif
}

const char* const kNames[kExpressionCount] = {
    "idle", "happy", "laugh", "wink", "curious", "thinking", "surprise", "smug",
    "sleepy", "sleep", "sad", "cry", "angry", "panic", "shy", "love"};

}  // namespace

const char* name(Expression expression) {
  const auto index = static_cast<uint8_t>(expression);
  return index < kExpressionCount ? kNames[index] : nullptr;
}

bool render(Expression expression, uint32_t elapsedMs,
            uint8_t* output, std::size_t outputSize) {
  const auto index = static_cast<uint8_t>(expression);
  if (index >= kExpressionCount || output == nullptr || outputSize < kFrameBytes) {
    return false;
  }

  const auto frame = (elapsedMs % kLoopDurationMs) / kFrameDurationMs;
  const auto uniqueFrame = readByte(&detail::kFrameIndices[index][frame]);
  if (uniqueFrame >= detail::kUniqueFrameCount) return false;

  const std::size_t begin = readWord(&detail::kFrameOffsets[uniqueFrame]);
  const std::size_t end = readWord(&detail::kFrameOffsets[uniqueFrame + 1]);
  if (begin > end || end > detail::kRleDataSize || (end - begin) % 2 != 0) {
    return false;
  }

  // Validate the complete frame before writing, so even corrupt assets leave
  // the caller's buffer unchanged. Every pair is an unsigned (count, value).
  std::size_t decodedSize = 0;
  for (std::size_t cursor = begin; cursor < end; cursor += 2) {
    const auto count = readByte(&detail::kRleData[cursor]);
    if (count == 0 || count > kFrameBytes - decodedSize) return false;
    decodedSize += count;
  }
  if (decodedSize != kFrameBytes) return false;

  std::size_t written = 0;
  for (std::size_t cursor = begin; cursor < end; cursor += 2) {
    const auto count = readByte(&detail::kRleData[cursor]);
    const auto value = readByte(&detail::kRleData[cursor + 1]);
    std::memset(output + written, value, count);
    written += count;
  }
  return true;
}

}  // namespace expressions
}  // namespace tiny_engineer
