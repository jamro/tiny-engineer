#include <unity.h>

#include <cstring>
#include <vector>

#include "serial/frame.h"

namespace {

struct Captured {
  std::vector<std::vector<uint8_t>> frames;
};

void onFrame(const uint8_t* payload, size_t length, void* user) {
  auto* cap = static_cast<Captured*>(user);
  cap->frames.emplace_back(payload, payload + length);
}

void test_round_trip_payload() {
  const uint8_t payload[] = {0x01, 0x00, 0x02, 0xFF};
  uint8_t encoded[64];
  const size_t n = serialFrameEncode(payload, sizeof(payload), encoded, sizeof(encoded));
  TEST_ASSERT_TRUE(n > 0);

  Captured cap;
  SerialFrameDecoder decoder;
  decoder.setHandler(onFrame, &cap);
  decoder.feed(encoded, n);

  TEST_ASSERT_EQUAL(1, cap.frames.size());
  TEST_ASSERT_EQUAL(sizeof(payload), cap.frames[0].size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, cap.frames[0].data(), sizeof(payload));
  TEST_ASSERT_FALSE(decoder.resyncing());
}

void test_resync_after_malformed() {
  const uint8_t good[] = {0xAA, 0xBB, 0xCC};
  uint8_t encoded[64];
  const size_t n = serialFrameEncode(good, sizeof(good), encoded, sizeof(encoded));
  TEST_ASSERT_TRUE(n > 0);

  // Code byte claims more data than follows before the delimiter.
  const uint8_t junk[] = {0x05, 0x00};

  Captured cap;
  SerialFrameDecoder decoder;
  decoder.setHandler(onFrame, &cap);
  decoder.feed(junk, sizeof(junk));
  TEST_ASSERT_TRUE(decoder.resyncing());
  TEST_ASSERT_EQUAL(0, cap.frames.size());

  decoder.feed(encoded, n);
  TEST_ASSERT_EQUAL(1, cap.frames.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(good, cap.frames[0].data(), sizeof(good));
  TEST_ASSERT_FALSE(decoder.resyncing());
}

void test_noise_then_valid() {
  const uint8_t payload[] = {7, 8, 9};
  uint8_t encoded[64];
  const size_t n = serialFrameEncode(payload, sizeof(payload), encoded, sizeof(encoded));

  Captured cap;
  SerialFrameDecoder decoder;
  decoder.setHandler(onFrame, &cap);

  // Noise must hit a delimiter so the decoder can drop it and resync.
  const uint8_t noise[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};
  decoder.feed(noise, sizeof(noise));
  decoder.feed(encoded, n);

  TEST_ASSERT_EQUAL(1, cap.frames.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, cap.frames[0].data(), sizeof(payload));
}

void test_rejects_oversized_payload_encode() {
  std::vector<uint8_t> huge(kSerialMaxPayload + 1, 0x11);
  uint8_t out[16];
  TEST_ASSERT_EQUAL(0, serialFrameEncode(huge.data(), huge.size(), out, sizeof(out)));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_round_trip_payload);
  RUN_TEST(test_resync_after_malformed);
  RUN_TEST(test_noise_then_valid);
  RUN_TEST(test_rejects_oversized_payload_encode);
  return UNITY_END();
}
