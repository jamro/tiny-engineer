#include <unity.h>

#include <cstring>
#include <string>

#include "serial/protocol.h"

namespace {

void test_pack_parse_round_trip() {
  const char body[] = "{\"op\":\"anim\",\"name\":\"ring\"}";
  SerialHeader header;
  header.type = SerialMsgType::Req;
  header.ver = kSerialProtoVersion;
  header.id = 0x1234;
  header.flags = 0;

  uint8_t packed[256];
  const size_t n = serialPackMessage(
    header,
    reinterpret_cast<const uint8_t*>(body),
    sizeof(body) - 1,
    packed,
    sizeof(packed)
  );
  TEST_ASSERT_EQUAL(kSerialHeaderSize + sizeof(body) - 1, n);

  SerialHeader parsed;
  size_t headerBytes = 0;
  TEST_ASSERT_TRUE(serialParseHeader(packed, n, parsed, headerBytes));
  TEST_ASSERT_EQUAL(kSerialHeaderSize, headerBytes);
  TEST_ASSERT_EQUAL(static_cast<uint8_t>(SerialMsgType::Req), static_cast<uint8_t>(parsed.type));
  TEST_ASSERT_EQUAL(kSerialProtoVersion, parsed.ver);
  TEST_ASSERT_EQUAL_HEX16(0x1234, parsed.id);
  TEST_ASSERT_EQUAL(sizeof(body) - 1, parsed.len);
  TEST_ASSERT_EQUAL_STRING_LEN(body, reinterpret_cast<const char*>(packed + kSerialHeaderSize), parsed.len);
}

void test_sync_json_includes_version_and_state() {
  char json[128];
  const size_t n = serialFormatSyncJson(true, "", json, sizeof(json));
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"proto\":1"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"provisioning\":true"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"link_mode\":\"\""));

  char jsonWifi[128];
  TEST_ASSERT_TRUE(serialFormatSyncJson(false, "wifi", jsonWifi, sizeof(jsonWifi)) > 0);
  TEST_ASSERT_NOT_NULL(std::strstr(jsonWifi, "\"provisioning\":false"));
  TEST_ASSERT_NOT_NULL(std::strstr(jsonWifi, "\"link_mode\":\"wifi\""));
}

void test_log_json_escapes() {
  char json[128];
  const size_t n = serialFormatLogJson("say \"hi\"\\ok", json, sizeof(json));
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_STRING("{\"t\":\"say \\\"hi\\\"\\\\ok\"}", json);
}

void test_sync_message_pack() {
  char json[128];
  const size_t jsonLen = serialFormatSyncJson(false, "usb", json, sizeof(json));
  TEST_ASSERT_TRUE(jsonLen > 0);

  SerialHeader header;
  header.type = SerialMsgType::Sync;
  header.id = 7;
  uint8_t packed[256];
  const size_t n = serialPackMessage(
    header,
    reinterpret_cast<const uint8_t*>(json),
    jsonLen,
    packed,
    sizeof(packed)
  );
  TEST_ASSERT_TRUE(n > kSerialHeaderSize);

  SerialHeader parsed;
  size_t headerBytes = 0;
  TEST_ASSERT_TRUE(serialParseHeader(packed, n, parsed, headerBytes));
  TEST_ASSERT_EQUAL(static_cast<uint8_t>(SerialMsgType::Sync), static_cast<uint8_t>(parsed.type));
  TEST_ASSERT_EQUAL(7, parsed.id);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pack_parse_round_trip);
  RUN_TEST(test_sync_json_includes_version_and_state);
  RUN_TEST(test_log_json_escapes);
  RUN_TEST(test_sync_message_pack);
  return UNITY_END();
}
