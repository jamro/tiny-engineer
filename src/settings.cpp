#include "settings.h"
#include "settings_internal.h"
#include "servos.h"

#include <cstring>

static_assert(
  SERVO_COUNT == SETTINGS_SERVO_COUNT,
  "SETTINGS_SERVO_COUNT must match SERVO_COUNT"
);

uint32_t settingsSleepTimeoutMin() {
  return g_sleepTimeoutMin;
}

uint32_t settingsSleepTimeoutMs() {
  return g_sleepTimeoutMin * 60UL * 1000UL;
}

const char* settingsHostname() {
  return g_hostname;
}

const char* settingsBootHostname() {
  return g_bootHostname;
}

uint8_t settingsVolume() {
  return g_volume;
}

bool settingsWelcomeEnabled() {
  return g_welcome;
}

bool settingsSerialLogEnabled() {
  return g_serialLog;
}

uint32_t settingsContinuousTimeoutMin() {
  return g_continuousTimeoutMin;
}

const char* settingsLoading() {
  return g_loading;
}

const char* settingsAccessToken() {
  return g_accessToken;
}

bool settingsAccessTokenSet() {
  return g_accessToken[0] != '\0';
}

const char* settingsWifiSsid() {
  return g_wifiSsid;
}

const char* settingsWifiPassword() {
  return g_wifiPassword;
}

bool settingsWifiConfigured() {
  return g_wifiSsid[0] != '\0';
}

bool settingsWifiPasswordSet() {
  return g_wifiPassword[0] != '\0';
}

float settingsServoMin(int index) {
  if (index < 0 || index >= static_cast<int>(SETTINGS_SERVO_COUNT)) {
    return 0.0f;
  }

  return static_cast<float>(g_servoMin[index]);
}

float settingsServoMax(int index) {
  if (index < 0 || index >= static_cast<int>(SETTINGS_SERVO_COUNT)) {
    return static_cast<float>(SETTINGS_SERVO_ANGLE_MAX);
  }

  return static_cast<float>(g_servoMax[index]);
}

const char* settingsRgbOrder() {
  return g_rgbOrder;
}

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutMin) {
  return sleepTimeoutMin >= SETTINGS_SLEEP_TIMEOUT_MIN_MIN &&
         sleepTimeoutMin <= SETTINGS_SLEEP_TIMEOUT_MAX_MIN;
}

bool settingsValidateHostname(const char* hostname) {
  if (hostname == nullptr) {
    return false;
  }

  const size_t len = strlen(hostname);

  if (len < 1 || len > SETTINGS_HOSTNAME_MAX_LEN) {
    return false;
  }

  if (hostname[0] == '-' || hostname[len - 1] == '-') {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    const char c = hostname[i];
    const bool ok =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-';

    if (!ok) {
      return false;
    }
  }

  return true;
}

bool settingsValidateVolume(uint8_t volume) {
  return volume <= SETTINGS_VOLUME_MAX;
}

bool settingsValidateContinuousTimeout(uint32_t continuousTimeoutMin) {
  return continuousTimeoutMin >= SETTINGS_CONTINUOUS_TIMEOUT_MIN_MIN &&
         continuousTimeoutMin <= SETTINGS_CONTINUOUS_TIMEOUT_MAX_MIN;
}

bool settingsValidateLoading(const char* loading) {
  if (loading == nullptr) {
    return false;
  }

  return strcmp(loading, "progress") == 0 ||
         strcmp(loading, "sleep_inertia") == 0;
}

bool settingsValidateAccessToken(const char* accessToken) {
  if (accessToken == nullptr) {
    return false;
  }

  const size_t len = strlen(accessToken);

  if (len > SETTINGS_ACCESS_TOKEN_MAX_LEN) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    const unsigned char c =
      static_cast<unsigned char>(accessToken[i]);

    if (c < 0x20 || c > 0x7E) {
      return false;
    }
  }

  return true;
}

bool settingsValidateWifiSsid(const char* wifiSsid) {
  if (wifiSsid == nullptr) {
    return false;
  }

  const size_t len = strlen(wifiSsid);

  return len >= 1 && len <= SETTINGS_WIFI_SSID_MAX_LEN;
}

bool settingsValidateWifiPassword(const char* wifiPassword) {
  if (wifiPassword == nullptr) {
    return false;
  }

  return strlen(wifiPassword) <= SETTINGS_WIFI_PASSWORD_MAX_LEN;
}

bool settingsValidateServoRanges(
  const uint8_t* servoMins,
  const uint8_t* servoMaxs
) {
  if (servoMins == nullptr || servoMaxs == nullptr) {
    return false;
  }

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (servoMins[i] > SETTINGS_SERVO_ANGLE_MAX ||
        servoMaxs[i] > SETTINGS_SERVO_ANGLE_MAX) {
      return false;
    }

    if (servoMins[i] >= servoMaxs[i]) {
      return false;
    }
  }

  return true;
}

bool settingsValidateRgbOrder(const char* rgbOrder) {
  if (rgbOrder == nullptr) {
    return false;
  }

  return strcmp(rgbOrder, "RGB") == 0 ||
         strcmp(rgbOrder, "RBG") == 0 ||
         strcmp(rgbOrder, "GRB") == 0 ||
         strcmp(rgbOrder, "GBR") == 0 ||
         strcmp(rgbOrder, "BRG") == 0 ||
         strcmp(rgbOrder, "BGR") == 0;
}
