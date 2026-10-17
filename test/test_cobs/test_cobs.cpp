#include <unity.h>

#include <cstring>
#include <vector>

#include <cobs.h>

namespace {

void roundTrip(const uint8_t* data, size_t len) {
  std::vector<cobs_byte_t> encoded(COBS_ENCODE_MAX(len));
  size_t encodedLen = 0;
  TEST_ASSERT_EQUAL(COBS_RET_SUCCESS, cobs_encode(data, len, encoded.data(), encoded.size(), &encodedLen));
  TEST_ASSERT_TRUE(encodedLen > 0);
  TEST_ASSERT_EQUAL(COBS_FRAME_DELIMITER, encoded[encodedLen - 1]);

  std::vector<cobs_byte_t> decoded(len + 8);
  size_t decodedLen = 0;
  size_t consumed = 0;
  TEST_ASSERT_EQUAL(
    COBS_RET_SUCCESS,
    cobs_decode(encoded.data(), encodedLen, decoded.data(), decoded.size(), &decodedLen, &consumed)
  );
  TEST_ASSERT_EQUAL(encodedLen, consumed);
  TEST_ASSERT_EQUAL(len, decodedLen);
  if (len > 0) {
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, decoded.data(), len);
  }
}

void test_empty_payload() {
  const uint8_t empty = 0;
  roundTrip(&empty, 0);
}

void test_sparse_zeros() {
  const uint8_t data[] = {1, 0, 2, 0, 0, 3, 4, 0, 5};
  roundTrip(data, sizeof(data));
}

void test_max_chunk_payload() {
  // Design max stream chunk payload is 1024 bytes; exercise that size.
  std::vector<uint8_t> data(1024);
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = static_cast<uint8_t>((i * 37u) & 0xFFu);
  }
  // Sprinkle zeros so COBS code bytes matter.
  data[0] = 0;
  data[253] = 0;
  data[508] = 0;
  data[1023] = 0;
  roundTrip(data.data(), data.size());
}

void test_no_zeros() {
  const uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  roundTrip(data, sizeof(data));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_empty_payload);
  RUN_TEST(test_sparse_zeros);
  RUN_TEST(test_max_chunk_payload);
  RUN_TEST(test_no_zeros);
  return UNITY_END();
}
