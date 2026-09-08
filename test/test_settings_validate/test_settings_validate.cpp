#include <cstring>
#include <unity.h>

#include "settings/settings.h"

void test_hostname_rejects_invalid() {
  TEST_ASSERT_FALSE(settingsValidateHostname(nullptr));
  TEST_ASSERT_FALSE(settingsValidateHostname(""));
  TEST_ASSERT_FALSE(settingsValidateHostname("-foo"));
  TEST_ASSERT_FALSE(settingsValidateHostname("foo-"));
  TEST_ASSERT_FALSE(settingsValidateHostname("foo_bar"));
  TEST_ASSERT_FALSE(
    settingsValidateHostname("abcdefghijklmnopqrstuvwxyz012345")
  );
}

void test_hostname_accepts_valid() {
  TEST_ASSERT_TRUE(settingsValidateHostname("tiny-engineer"));
  TEST_ASSERT_TRUE(
    settingsValidateHostname("abcdefghijklmnopqrstuvwxyz01234")
  );
  TEST_ASSERT_TRUE(settingsValidateHostname("Foo-1"));
}

void test_sleep_timeout() {
  TEST_ASSERT_FALSE(settingsValidateSleepTimeout(0));
  TEST_ASSERT_TRUE(settingsValidateSleepTimeout(1));
  TEST_ASSERT_TRUE(settingsValidateSleepTimeout(1440));
  TEST_ASSERT_FALSE(settingsValidateSleepTimeout(1441));
}

void test_continuous_timeout() {
  TEST_ASSERT_FALSE(settingsValidateContinuousTimeout(0));
  TEST_ASSERT_TRUE(settingsValidateContinuousTimeout(1));
  TEST_ASSERT_TRUE(settingsValidateContinuousTimeout(1440));
  TEST_ASSERT_FALSE(settingsValidateContinuousTimeout(1441));
}

void test_volume() {
  TEST_ASSERT_TRUE(settingsValidateVolume(0));
  TEST_ASSERT_TRUE(settingsValidateVolume(100));
  TEST_ASSERT_FALSE(settingsValidateVolume(101));
}

void test_loading() {
  TEST_ASSERT_TRUE(settingsValidateLoading("progress"));
  TEST_ASSERT_TRUE(settingsValidateLoading("sleep_inertia"));
  TEST_ASSERT_FALSE(settingsValidateLoading("other"));
  TEST_ASSERT_FALSE(settingsValidateLoading(nullptr));
}

void test_access_token() {
  char token64[65];
  char token65[66];

  memset(token64, 'a', 64);
  token64[64] = '\0';
  memset(token65, 'a', 65);
  token65[65] = '\0';

  TEST_ASSERT_TRUE(settingsValidateAccessToken(""));
  TEST_ASSERT_TRUE(settingsValidateAccessToken(token64));
  TEST_ASSERT_FALSE(settingsValidateAccessToken(token65));
  TEST_ASSERT_FALSE(settingsValidateAccessToken("foo\x01" "bar"));
  TEST_ASSERT_FALSE(settingsValidateAccessToken(nullptr));
}

void test_wifi_ssid() {
  char ssid32[33];
  char ssid33[34];

  memset(ssid32, 's', 32);
  ssid32[32] = '\0';
  memset(ssid33, 's', 33);
  ssid33[33] = '\0';

  TEST_ASSERT_FALSE(settingsValidateWifiSsid(""));
  TEST_ASSERT_FALSE(settingsValidateWifiSsid(ssid33));
  TEST_ASSERT_FALSE(settingsValidateWifiSsid(nullptr));
  TEST_ASSERT_TRUE(settingsValidateWifiSsid("a"));
  TEST_ASSERT_TRUE(settingsValidateWifiSsid(ssid32));
}

void test_wifi_password() {
  char pass63[64];
  char pass64[65];

  memset(pass63, 'p', 63);
  pass63[63] = '\0';
  memset(pass64, 'p', 64);
  pass64[64] = '\0';

  TEST_ASSERT_TRUE(settingsValidateWifiPassword(""));
  TEST_ASSERT_TRUE(settingsValidateWifiPassword(pass63));
  TEST_ASSERT_FALSE(settingsValidateWifiPassword(pass64));
  TEST_ASSERT_FALSE(settingsValidateWifiPassword(nullptr));
}

void test_servo_ranges() {
  const uint8_t validMin[SETTINGS_SERVO_COUNT] = {10, 20, 30, 40, 50};
  const uint8_t validMax[SETTINGS_SERVO_COUNT] = {20, 30, 40, 50, 60};
  const uint8_t equalMin[SETTINGS_SERVO_COUNT] = {10, 20, 30, 40, 50};
  const uint8_t equalMax[SETTINGS_SERVO_COUNT] = {10, 30, 40, 50, 60};
  const uint8_t overMin[SETTINGS_SERVO_COUNT] = {10, 20, 30, 40, 50};
  const uint8_t overMax[SETTINGS_SERVO_COUNT] = {20, 30, 40, 50, 181};

  TEST_ASSERT_FALSE(settingsValidateServoRanges(nullptr, validMax));
  TEST_ASSERT_FALSE(settingsValidateServoRanges(validMin, nullptr));
  TEST_ASSERT_FALSE(settingsValidateServoRanges(equalMin, equalMax));
  TEST_ASSERT_FALSE(settingsValidateServoRanges(overMin, overMax));
  TEST_ASSERT_TRUE(settingsValidateServoRanges(validMin, validMax));
}

void test_rgb_order() {
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("RGB"));
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("RBG"));
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("GRB"));
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("GBR"));
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("BRG"));
  TEST_ASSERT_TRUE(settingsValidateRgbOrder("BGR"));
  TEST_ASSERT_FALSE(settingsValidateRgbOrder("RGBG"));
  TEST_ASSERT_FALSE(settingsValidateRgbOrder("grb"));
  TEST_ASSERT_FALSE(settingsValidateRgbOrder(nullptr));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_hostname_rejects_invalid);
  RUN_TEST(test_hostname_accepts_valid);
  RUN_TEST(test_sleep_timeout);
  RUN_TEST(test_continuous_timeout);
  RUN_TEST(test_volume);
  RUN_TEST(test_loading);
  RUN_TEST(test_access_token);
  RUN_TEST(test_wifi_ssid);
  RUN_TEST(test_wifi_password);
  RUN_TEST(test_servo_ranges);
  RUN_TEST(test_rgb_order);
  return UNITY_END();
}
