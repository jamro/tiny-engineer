#include "settings/internal.h"
#include "settings/nvs.h"
#include "serial_log.h"
#include "settings/settings.h"

#include <cstring>

bool saveSettings(
  const uint32_t* sleepTimeoutMin,
  const char* hostname,
  const uint8_t* volume,
  const bool* welcome,
  const bool* serialLog,
  const bool* servoHold,
  const bool* wifiEnabled,
  const uint32_t* continuousTimeoutMin,
  const char* loading,
  const char* eyesStyle,
  const char* accessToken,
  const uint8_t* servoMins,
  const uint8_t* servoMaxs,
  const char* rgbOrder,
  const bool* oledRotate180,
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
      servoHold == nullptr &&
      wifiEnabled == nullptr &&
      continuousTimeoutMin == nullptr &&
      loading == nullptr &&
      eyesStyle == nullptr &&
      accessToken == nullptr &&
      servoMins == nullptr &&
      servoMaxs == nullptr &&
      rgbOrder == nullptr &&
      oledRotate180 == nullptr) {
    return false;
  }

  uint32_t nextSleep = g_sleepTimeoutMin;
  char nextHost[SETTINGS_HOSTNAME_MAX_LEN + 1];
  setHostnameCache(nextHost, g_hostname);
  uint8_t nextVolume = g_volume;
  bool nextWelcome = g_welcome;
  bool nextSerialLog = g_serialLog;
  bool nextServoHold = g_servoHold;
  bool nextWifiEnabled = g_wifiEnabled;
  uint32_t nextContTo = g_continuousTimeoutMin;
  char nextLoading[SETTINGS_LOADING_MAX_LEN + 1];
  setLoadingCache(nextLoading, g_loading);
  char nextEyesStyle[SETTINGS_EYES_STYLE_MAX_LEN + 1];
  setEyesStyleCache(nextEyesStyle, g_eyesStyle);
  char nextAccessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1];
  setAccessTokenCache(nextAccessToken, g_accessToken);
  uint8_t nextServoMin[SETTINGS_SERVO_COUNT];
  uint8_t nextServoMax[SETTINGS_SERVO_COUNT];
  memcpy(nextServoMin, g_servoMin, SETTINGS_SERVO_COUNT);
  memcpy(nextServoMax, g_servoMax, SETTINGS_SERVO_COUNT);
  char nextRgbOrder[SETTINGS_RGB_ORDER_MAX_LEN + 1];
  setRgbOrderCache(nextRgbOrder, g_rgbOrder);
  bool nextOledRotate180 = g_oledRotate180;

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

  if (servoHold != nullptr) {
    nextServoHold = *servoHold;
  }

  if (wifiEnabled != nullptr) {
    nextWifiEnabled = *wifiEnabled;
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

  if (eyesStyle != nullptr) {
    if (!settingsValidateEyesStyle(eyesStyle)) {
      return false;
    }

    setEyesStyleCache(nextEyesStyle, eyesStyle);
  }

  if (accessToken != nullptr) {
    if (!settingsValidateAccessToken(accessToken)) {
      return false;
    }

    setAccessTokenCache(nextAccessToken, accessToken);
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

  if (oledRotate180 != nullptr) {
    nextOledRotate180 = *oledRotate180;
  }

  if (!writeAllToNvs(
        nextSleep,
        nextHost,
        nextVolume,
        nextWelcome,
        nextSerialLog,
        nextServoHold,
        nextWifiEnabled,
        nextContTo,
        nextLoading,
        nextEyesStyle,
        nextAccessToken,
        nextServoMin,
        nextServoMax,
        nextRgbOrder,
        nextOledRotate180
      )) {
    return false;
  }

  g_sleepTimeoutMin = nextSleep;
  setHostnameCache(g_hostname, nextHost);
  g_volume = nextVolume;
  g_welcome = nextWelcome;
  g_serialLog = nextSerialLog;
  g_servoHold = nextServoHold;
  g_wifiEnabled = nextWifiEnabled;
  g_continuousTimeoutMin = nextContTo;
  setLoadingCache(g_loading, nextLoading);
  setEyesStyleCache(g_eyesStyle, nextEyesStyle);
  setAccessTokenCache(g_accessToken, nextAccessToken);
  memcpy(g_servoMin, nextServoMin, SETTINGS_SERVO_COUNT);
  memcpy(g_servoMax, nextServoMax, SETTINGS_SERVO_COUNT);
  setRgbOrderCache(g_rgbOrder, nextRgbOrder);
  g_oledRotate180 = nextOledRotate180;

  if (rebootRequired != nullptr) {
    if (strcmp(g_hostname, g_bootHostname) != 0 ||
        g_wifiEnabled != g_bootWifiEnabled) {
      *rebootRequired = true;
    }
  }

  logSettingsSnapshot("Settings saved:");
  return true;
}

bool saveWifiNetworks(const WifiNetworkList& networks) {
  if (!settingsNvsBeginWrite()) {
    serialLogPrintln("Settings: NVS write open failed");
    return false;
  }

  const size_t length = networks.count() * sizeof(WifiNetwork);
  const bool written =
    g_settingsPrefs.putBytes(kKeyWifiNets, networks.data(), length) == length;

  settingsNvsEnd();

  if (!written) {
    serialLogPrintln("Settings: WiFi networks write failed");
    return false;
  }

  g_wifiNetworks = networks;
  logSettingsSnapshot("Settings saved:");
  return true;
}
