#include "settings/internal.h"
#include "settings/nvs.h"
#include "settings/settings.h"

#include <cstring>

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
  bool* rebootRequired
) {
  if (rebootRequired != nullptr) {
    *rebootRequired = false;
  }

  if (sleepTimeoutMin == nullptr &&
      hostname == nullptr &&
      volume == nullptr &&
      welcome == nullptr &&
      serialLog == nullptr &&
      continuousTimeoutMin == nullptr &&
      loading == nullptr &&
      accessToken == nullptr &&
      wifiSsid == nullptr &&
      wifiPassword == nullptr &&
      servoMins == nullptr &&
      servoMaxs == nullptr &&
      rgbOrder == nullptr) {
    return false;
  }

  uint32_t nextSleep = g_sleepTimeoutMin;
  char nextHost[SETTINGS_HOSTNAME_MAX_LEN + 1];
  setHostnameCache(nextHost, g_hostname);
  uint8_t nextVolume = g_volume;
  bool nextWelcome = g_welcome;
  bool nextSerialLog = g_serialLog;
  uint32_t nextContTo = g_continuousTimeoutMin;
  char nextLoading[SETTINGS_LOADING_MAX_LEN + 1];
  setLoadingCache(nextLoading, g_loading);
  char nextAccessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1];
  setAccessTokenCache(nextAccessToken, g_accessToken);
  char nextWifiSsid[SETTINGS_WIFI_SSID_MAX_LEN + 1];
  setWifiSsidCache(nextWifiSsid, g_wifiSsid);
  char nextWifiPassword[SETTINGS_WIFI_PASSWORD_MAX_LEN + 1];
  setWifiPasswordCache(nextWifiPassword, g_wifiPassword);
  uint8_t nextServoMin[SETTINGS_SERVO_COUNT];
  uint8_t nextServoMax[SETTINGS_SERVO_COUNT];
  memcpy(nextServoMin, g_servoMin, SETTINGS_SERVO_COUNT);
  memcpy(nextServoMax, g_servoMax, SETTINGS_SERVO_COUNT);
  char nextRgbOrder[SETTINGS_RGB_ORDER_MAX_LEN + 1];
  setRgbOrderCache(nextRgbOrder, g_rgbOrder);

  if (sleepTimeoutMin != nullptr) {
    if (!settingsValidateSleepTimeout(*sleepTimeoutMin)) {
      return false;
    }

    nextSleep = *sleepTimeoutMin;
  }

  if (hostname != nullptr) {
    if (!settingsValidateHostname(hostname)) {
      return false;
    }

    setHostnameCache(nextHost, hostname);
  }

  if (volume != nullptr) {
    if (!settingsValidateVolume(*volume)) {
      return false;
    }

    nextVolume = *volume;
  }

  if (welcome != nullptr) {
    nextWelcome = *welcome;
  }

  if (serialLog != nullptr) {
    nextSerialLog = *serialLog;
  }

  if (continuousTimeoutMin != nullptr) {
    if (!settingsValidateContinuousTimeout(*continuousTimeoutMin)) {
      return false;
    }

    nextContTo = *continuousTimeoutMin;
  }

  if (loading != nullptr) {
    if (!settingsValidateLoading(loading)) {
      return false;
    }

    setLoadingCache(nextLoading, loading);
  }

  if (accessToken != nullptr) {
    if (!settingsValidateAccessToken(accessToken)) {
      return false;
    }

    setAccessTokenCache(nextAccessToken, accessToken);
  }

  if (wifiSsid != nullptr) {
    if (!settingsValidateWifiSsid(wifiSsid)) {
      return false;
    }

    setWifiSsidCache(nextWifiSsid, wifiSsid);
  }

  if (wifiPassword != nullptr) {
    if (!settingsValidateWifiPassword(wifiPassword)) {
      return false;
    }

    setWifiPasswordCache(nextWifiPassword, wifiPassword);
  }

  if (servoMins != nullptr || servoMaxs != nullptr) {
    if (servoMins == nullptr || servoMaxs == nullptr) {
      return false;
    }

    if (!settingsValidateServoRanges(servoMins, servoMaxs)) {
      return false;
    }

    memcpy(nextServoMin, servoMins, SETTINGS_SERVO_COUNT);
    memcpy(nextServoMax, servoMaxs, SETTINGS_SERVO_COUNT);
  }

  if (rgbOrder != nullptr) {
    if (!settingsValidateRgbOrder(rgbOrder)) {
      return false;
    }

    setRgbOrderCache(nextRgbOrder, rgbOrder);
  }

  if (!writeAllToNvs(
        nextSleep,
        nextHost,
        nextVolume,
        nextWelcome,
        nextSerialLog,
        nextContTo,
        nextLoading,
        nextAccessToken,
        nextWifiSsid,
        nextWifiPassword,
        nextServoMin,
        nextServoMax,
        nextRgbOrder
      )) {
    return false;
  }

  g_sleepTimeoutMin = nextSleep;
  setHostnameCache(g_hostname, nextHost);
  g_volume = nextVolume;
  g_welcome = nextWelcome;
  g_serialLog = nextSerialLog;
  g_continuousTimeoutMin = nextContTo;
  setLoadingCache(g_loading, nextLoading);
  setAccessTokenCache(g_accessToken, nextAccessToken);
  setWifiSsidCache(g_wifiSsid, nextWifiSsid);
  setWifiPasswordCache(g_wifiPassword, nextWifiPassword);
  memcpy(g_servoMin, nextServoMin, SETTINGS_SERVO_COUNT);
  memcpy(g_servoMax, nextServoMax, SETTINGS_SERVO_COUNT);
  setRgbOrderCache(g_rgbOrder, nextRgbOrder);

  if (rebootRequired != nullptr &&
      strcmp(g_hostname, g_bootHostname) != 0) {
    *rebootRequired = true;
  }

  logSettingsSnapshot("Settings saved:");
  return true;
}
