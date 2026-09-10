#include "settings/settings.h"
#include "settings/internal.h"
#include "servos.h"

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

bool settingsOledRotate180() {
  return g_oledRotate180;
}
