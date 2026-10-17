#include "settings/settings.h"
#include "settings/values.h"
#include "servos.h"

static_assert(
  SERVO_COUNT == SETTINGS_SERVO_COUNT,
  "SETTINGS_SERVO_COUNT must match SERVO_COUNT"
);

uint32_t settingsSleepTimeoutMin() {
  return g_settings.sleepTimeoutMin;
}

uint32_t settingsSleepTimeoutMs() {
  return g_settings.sleepTimeoutMin * 60UL * 1000UL;
}

const char* settingsHostname() {
  return g_settings.hostname;
}

uint8_t settingsVolume() {
  return g_settings.volume;
}

bool settingsWelcomeEnabled() {
  return g_settings.welcome;
}

bool settingsSerialLogEnabled() {
  return g_settings.serialLog;
}

uint32_t settingsContinuousTimeoutMin() {
  return g_settings.continuousTimeoutMin;
}

const char* settingsLoading() {
  return g_settings.loading;
}

const char* settingsEyesStyle() {
  return g_settings.eyesStyle;
}

const char* settingsAccessToken() {
  return g_settings.accessToken;
}

bool settingsAccessTokenSet() {
  return g_settings.accessToken[0] != '\0';
}

const WifiNetworkList& settingsWifiNetworks() {
  return g_wifiNetworks;
}

bool settingsWifiConfigured() {
  return g_wifiNetworks.count() > 0;
}

const char* settingsLinkMode() {
  return g_settings.linkMode;
}

bool settingsProvisioned() {
  return strcmp(g_settings.linkMode, "wifi") == 0 || strcmp(g_settings.linkMode, "usb") == 0;
}

float settingsServoMin(int index) {
  if (index < 0 || index >= static_cast<int>(SETTINGS_SERVO_COUNT)) {
    return 0.0f;
  }

  return static_cast<float>(g_settings.servoRanges.mins[index]);
}

float settingsServoMax(int index) {
  if (index < 0 || index >= static_cast<int>(SETTINGS_SERVO_COUNT)) {
    return static_cast<float>(SETTINGS_SERVO_ANGLE_MAX);
  }

  return static_cast<float>(g_settings.servoRanges.maxs[index]);
}

const char* settingsRgbOrder() {
  return g_settings.rgbOrder;
}

bool settingsOledRotate180() {
  return g_settings.oledRotate180;
}
