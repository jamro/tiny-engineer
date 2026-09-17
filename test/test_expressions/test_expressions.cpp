// SPDX-License-Identifier: MIT
#include <unity.h>
#include <TinyEngineerExpressions.h>
#include <cstring>
#include <limits>

namespace expr = tiny_engineer::expressions;

void test_names_and_invalid_ids() {
  const char* expected[] = {
    "idle", "happy", "laugh", "wink", "curious", "thinking", "surprise", "smug",
    "sleepy", "sleep", "sad", "cry", "angry", "panic", "shy", "love"
  };
  TEST_ASSERT_EQUAL_UINT32(16, expr::kExpressionCount);
  for (size_t i = 0; i < expr::kExpressionCount; ++i) {
    TEST_ASSERT_EQUAL_STRING(expected[i], expr::name(static_cast<expr::Expression>(i)));
  }
  for (unsigned i = 16; i <= 255; ++i) {
    TEST_ASSERT_NULL(expr::name(static_cast<expr::Expression>(i)));
  }
}

void test_all_frames_match_original_designs() {
  // FNV-1a over each original uncompressed 30-frame animation, before RLE.
  // scripts/expressions/test-assets.js separately pins their SHA-256 digests.
  const uint32_t expected[] = {
    0x019fd60du, 0xa5101635u, 0xe2919a55u, 0xfb7f1da2u,
    0x86be37c5u, 0x9988e8a7u, 0x1fdf4c81u, 0x89025fa9u,
    0xc9ab6b4eu, 0x8bdc177du, 0x572f1dddu, 0x2432422du,
    0xe9242f32u, 0x4efb79e5u, 0x8b0d926du, 0x2bde1b40u
  };
  uint8_t guarded[expr::kFrameBytes + 2];
  for (size_t e = 0; e < expr::kExpressionCount; ++e) {
    uint32_t hash = 2166136261u;
    for (size_t f = 0; f < expr::kFramesPerExpression; ++f) {
      std::memset(guarded, 0xA5, sizeof guarded);
      TEST_ASSERT_TRUE(expr::render(static_cast<expr::Expression>(e),
        f * expr::kFrameDurationMs, guarded + 1, expr::kFrameBytes));
      TEST_ASSERT_EQUAL_HEX8(0xA5, guarded[0]);
      TEST_ASSERT_EQUAL_HEX8(0xA5, guarded[sizeof guarded - 1]);
      for (size_t i = 1; i <= expr::kFrameBytes; ++i) {
        hash = (hash ^ guarded[i]) * 16777619u;
      }
    }
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(expected[e], hash, expr::name(static_cast<expr::Expression>(e)));
  }
}

void test_invalid_inputs_leave_buffer_untouched() {
  uint8_t output[expr::kFrameBytes + 2], before[sizeof output];
  std::memset(output, 0xA5, sizeof output);
  std::memcpy(before, output, sizeof output);
  TEST_ASSERT_FALSE(expr::render(expr::Expression::Idle, 0, nullptr, expr::kFrameBytes));
  for (size_t size = 0; size < expr::kFrameBytes; ++size) {
    TEST_ASSERT_FALSE(expr::render(expr::Expression::Idle, 0, output, size));
  }
  for (unsigned id = 16; id <= 255; ++id) {
    TEST_ASSERT_FALSE(expr::render(static_cast<expr::Expression>(id), 0, output, sizeof output));
  }
  TEST_ASSERT_EQUAL_UINT8_ARRAY(before, output, sizeof output);
  // A larger capacity is accepted; only the first 512 bytes are written.
  TEST_ASSERT_TRUE(expr::render(expr::Expression::Idle, 0, output, sizeof output));
  TEST_ASSERT_EQUAL_HEX8(0xA5, output[expr::kFrameBytes]);
  TEST_ASSERT_EQUAL_HEX8(0xA5, output[expr::kFrameBytes + 1]);
}

void test_frame_timing_and_loop_boundaries() {
  uint8_t frame[expr::kFrameBytes], reference[expr::kFrameBytes];
  for (size_t e = 0; e < expr::kExpressionCount; ++e) {
    const auto id = static_cast<expr::Expression>(e);
    for (uint32_t f = 0; f < expr::kFramesPerExpression; ++f) {
      TEST_ASSERT_TRUE(expr::render(id, f * 100, reference, sizeof reference));
      TEST_ASSERT_TRUE(expr::render(id, f * 100 + 99, frame, sizeof frame));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(reference, frame, sizeof frame);
      TEST_ASSERT_TRUE(expr::render(id, 12000 + f * 100, frame, sizeof frame));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(reference, frame, sizeof frame);
    }
    const uint32_t last = std::numeric_limits<uint32_t>::max();
    TEST_ASSERT_TRUE(expr::render(id, last % 3000, reference, sizeof reference));
    TEST_ASSERT_TRUE(expr::render(id, last, frame, sizeof frame));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reference, frame, sizeof frame);
  }
}

void test_callers_can_render_independently() {
  uint8_t first[expr::kFrameBytes], second[expr::kFrameBytes], again[expr::kFrameBytes];
  TEST_ASSERT_TRUE(expr::render(expr::Expression::Love, 900, first, sizeof first));
  TEST_ASSERT_TRUE(expr::render(expr::Expression::Sleep, 1200, second, sizeof second));
  TEST_ASSERT_TRUE(expr::render(expr::Expression::Love, 900, again, sizeof again));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(first, again, sizeof first);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_names_and_invalid_ids);
  RUN_TEST(test_all_frames_match_original_designs);
  RUN_TEST(test_invalid_inputs_leave_buffer_untouched);
  RUN_TEST(test_frame_timing_and_loop_boundaries);
  RUN_TEST(test_callers_can_render_independently);
  return UNITY_END();
}
