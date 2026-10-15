#include <cstring>
#include <unity.h>
#include <limits>

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

void test_eyes_style() {
  TEST_ASSERT_TRUE(settingsValidateEyesStyle("classic"));
  TEST_ASSERT_TRUE(settingsValidateEyesStyle("kaomoji"));
  TEST_ASSERT_TRUE(settingsValidateEyesStyle("cover"));
  TEST_ASSERT_TRUE(settingsValidateEyesStyle("dots"));
  TEST_ASSERT_FALSE(settingsValidateEyesStyle("other"));
  TEST_ASSERT_FALSE(settingsValidateEyesStyle(nullptr));
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
  TEST_ASSERT_TRUE(settingsValidateAccessToken("two words"));
  TEST_ASSERT_FALSE(settingsValidateAccessToken(" leading"));
  TEST_ASSERT_FALSE(settingsValidateAccessToken("trailing "));
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
  const ServoRanges valid = {{10, 20, 30, 40, 50}, {20, 30, 40, 50, 180}};
  const ServoRanges equal = {{10, 20, 30, 40, 50}, {20, 30, 40, 50, 50}};
  const ServoRanges over = {{10, 20, 30, 40, 50}, {20, 30, 40, 50, 181}};

  TEST_ASSERT_TRUE(valid.valid());
  TEST_ASSERT_FALSE(equal.valid());
  TEST_ASSERT_FALSE(over.valid());
}

void test_servo_centers_reject_invalid_angles() {
  const ServoRanges ranges = {{60, 40, 45, 35, 40}, {130, 130, 135, 125, 130}};
  ServoCenters centers = {{95, 85, 90, 80, 85}};

  TEST_ASSERT_TRUE(centers.valid(&ranges));
  centers.angles[4] = 85.5f;
  TEST_ASSERT_TRUE(centers.valid(&ranges));

  for (const float invalid : {40.0f, 130.0f, -1.0f, 181.0f,
                             std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<float>::infinity()}) {
    centers.angles[4] = invalid;
    TEST_ASSERT_FALSE(centers.valid(&ranges));
  }
}

void test_range_changes_keep_centers_or_fall_back_to_midpoint() {
  const ServoRanges ranges = {{60, 40, 45, 35, 40}, {131, 130, 135, 125, 130}};
  ServoCenters centers = {{60, 85, 135, 80, std::numeric_limits<float>::quiet_NaN()}};

  centers.fitTo(ranges);

  TEST_ASSERT_TRUE(centers.valid(&ranges));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 95.5f, centers.angles[0]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 85.0f, centers.angles[1]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, centers.angles[2]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 80.0f, centers.angles[3]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 85.0f, centers.angles[4]);
}

void test_patch_validates_centers_against_resulting_ranges() {
  const ServoRanges saved = {{60, 40, 45, 35, 40}, {130, 130, 135, 125, 130}};
  SettingsPatch patch;
  patch.servoCenters = ServoCenters{{95, 85, 90, 80, 35}};

  TEST_ASSERT_EQUAL_STRING("servo_centers", patch.invalidField(&saved));
  TEST_ASSERT_NULL(patch.setupOnlyStep());

  // A center can move outside the old band only when the new band allows it.
  patch.servoRanges = ServoRanges{{60, 40, 45, 35, 30}, {130, 130, 135, 125, 130}};
  TEST_ASSERT_NULL(patch.invalidField(&saved));
  TEST_ASSERT_NULL(patch.setupOnlyStep());

  patch.servoCenters->angles[4] = 30;
  TEST_ASSERT_EQUAL_STRING("servo_centers", patch.invalidField(&saved));
}

void test_patch_names_the_first_invalid_field() {
  SettingsPatch patch;

  TEST_ASSERT_NULL(patch.invalidField());

  patch.volume = 50;
  patch.hostname = "tiny-engineer";
  TEST_ASSERT_NULL(patch.invalidField());

  patch.volume = 101;
  patch.rgbOrder = "XYZ";
  TEST_ASSERT_EQUAL_STRING("volume", patch.invalidField());

  patch.volume = 100;
  TEST_ASSERT_EQUAL_STRING("rgb_order", patch.invalidField());
}

void test_patch_names_its_setup_only_step() {
  SettingsPatch patch;

  patch.volume = 50;
  patch.hostname = "tiny-engineer";
  patch.swapHands = true;
  TEST_ASSERT_NULL(patch.setupOnlyStep());

  patch.servoRanges = ServoRanges{{10, 20, 30, 40, 50}, {20, 30, 40, 50, 60}};
  TEST_ASSERT_NULL(patch.setupOnlyStep());

  patch.oledRotate180 = false;
  TEST_ASSERT_EQUAL_STRING("oled", patch.setupOnlyStep());

  patch.rgbOrder = "GRB";
  TEST_ASSERT_EQUAL_STRING("rgb", patch.setupOnlyStep());
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
  RUN_TEST(test_eyes_style);
  RUN_TEST(test_access_token);
  RUN_TEST(test_wifi_ssid);
  RUN_TEST(test_wifi_password);
  RUN_TEST(test_servo_ranges);
  RUN_TEST(test_servo_centers_reject_invalid_angles);
  RUN_TEST(test_range_changes_keep_centers_or_fall_back_to_midpoint);
  RUN_TEST(test_patch_validates_centers_against_resulting_ranges);
  RUN_TEST(test_patch_names_the_first_invalid_field);
  RUN_TEST(test_patch_names_its_setup_only_step);
  RUN_TEST(test_rgb_order);
  return UNITY_END();
}
