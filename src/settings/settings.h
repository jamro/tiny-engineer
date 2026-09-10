#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint32_t SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN = 10;
constexpr const char* SETTINGS_DEFAULT_HOSTNAME = "tiny-engineer";
constexpr uint8_t SETTINGS_DEFAULT_VOLUME = 70;
constexpr bool SETTINGS_DEFAULT_WELCOME = true;
constexpr bool SETTINGS_DEFAULT_SERIAL_LOG = false;
constexpr uint32_t SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN = 5;
constexpr const char* SETTINGS_DEFAULT_LOADING = "progress";
constexpr const char* SETTINGS_DEFAULT_ACCESS_TOKEN = "";

constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MIN_MIN = 1;
constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MAX_MIN = 1440;
constexpr size_t SETTINGS_HOSTNAME_MAX_LEN = 31;
constexpr size_t SETTINGS_LOADING_MAX_LEN = 15;
constexpr size_t SETTINGS_ACCESS_TOKEN_MAX_LEN = 64;
constexpr size_t SETTINGS_WIFI_SSID_MAX_LEN = 32;
constexpr size_t SETTINGS_WIFI_PASSWORD_MAX_LEN = 63;
constexpr uint8_t SETTINGS_VOLUME_MIN = 0;
constexpr uint8_t SETTINGS_VOLUME_MAX = 100;
constexpr uint32_t SETTINGS_CONTINUOUS_TIMEOUT_MIN_MIN = 1;
constexpr uint32_t SETTINGS_CONTINUOUS_TIMEOUT_MAX_MIN = 1440;
constexpr size_t SETTINGS_SERVO_COUNT = 5;
constexpr uint8_t SETTINGS_SERVO_ANGLE_MAX = 180;
constexpr size_t SETTINGS_RGB_ORDER_MAX_LEN = 3;
constexpr const char* SETTINGS_DEFAULT_RGB_ORDER = "GRB";
constexpr bool SETTINGS_DEFAULT_OLED_ROTATE_180 = false;

void initSettings();

uint32_t settingsSleepTimeoutMin();
uint32_t settingsSleepTimeoutMs();
const char* settingsHostname();
uint8_t settingsVolume();
bool settingsWelcomeEnabled();
bool settingsSerialLogEnabled();
uint32_t settingsContinuousTimeoutMin();
const char* settingsLoading();
const char* settingsAccessToken();
bool settingsAccessTokenSet();
const char* settingsWifiSsid();
const char* settingsWifiPassword();
bool settingsWifiConfigured();
bool settingsWifiPasswordSet();
float settingsServoMin(int index);
float settingsServoMax(int index);
const char* settingsRgbOrder();
bool settingsOledRotate180();

// Hostname used at boot for Wi-Fi/mDNS (frozen after initSettings).
const char* settingsBootHostname();

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutMin);
bool settingsValidateHostname(const char* hostname);
bool settingsValidateVolume(uint8_t volume);
bool settingsValidateContinuousTimeout(uint32_t continuousTimeoutMin);
bool settingsValidateLoading(const char* loading);
bool settingsValidateAccessToken(const char* accessToken);
bool settingsValidateWifiSsid(const char* wifiSsid);
bool settingsValidateWifiPassword(const char* wifiPassword);
bool settingsValidateServoRanges(
  const uint8_t* servoMins,
  const uint8_t* servoMaxs
);
bool settingsValidateRgbOrder(const char* rgbOrder);

// Updates RAM cache and NVS for any non-null args. Returns false on validation
// failure (no write). If hostname is set and differs from boot hostname,
// *rebootRequired is set true when non-null.
bool saveSettings(
  const uint32_t* sleepTimeoutMin,
  const char* hostname,
  const uint8_t* volume,
  const bool* welcome,
  const bool* serialLog,
  const uint32_t* continuousTimeoutMin,
  const char* loading,
  const char* accessToken,
  const char* wifiSsid,
  const char* wifiPassword,
  const uint8_t* servoMins,
  const uint8_t* servoMaxs,
  const char* rgbOrder,
  const bool* oledRotate180,
  bool* rebootRequired
);

// Clears NVS namespace and restores settings to defaults except servo
// min/max, RGB LED byte order, and OLED 180° rotation, which stay as
// currently saved.
// *rebootRequired is set when boot hostname or loading differed from
// defaults before reset.
bool factoryResetSettings(bool* rebootRequired);
