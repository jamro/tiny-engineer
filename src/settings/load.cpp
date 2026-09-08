#include "settings/internal.h"
#include "settings/nvs.h"
#include "serial_log.h"
#include "settings/settings.h"

#include <Arduino.h>
#include <cstring>

void initSettings() {
  setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  g_volume = SETTINGS_DEFAULT_VOLUME;
  g_welcome = SETTINGS_DEFAULT_WELCOME;
  g_serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
  g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  setLoadingCache(g_loading, SETTINGS_DEFAULT_LOADING);
  setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  setWifiSsidCache(g_wifiSsid, "");
  setWifiPasswordCache(g_wifiPassword, "");
  fillDefaultServoRanges(g_servoMin, g_servoMax);
  setRgbOrderCache(g_rgbOrder, SETTINGS_DEFAULT_RGB_ORDER);

  if (!settingsNvsBeginRead()) {
    serialLogPrintln("Settings: NVS open failed; using defaults");
    setHostnameCache(g_bootHostname, g_hostname);
    return;
  }

  g_sleepTimeoutMin = g_settingsPrefs.getUInt(
    kKeySleep,
    SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN
  );

  if (!settingsValidateSleepTimeout(g_sleepTimeoutMin)) {
    g_sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
  }

  String host = g_settingsPrefs.getString(kKeyHost, SETTINGS_DEFAULT_HOSTNAME);

  if (settingsValidateHostname(host.c_str())) {
    setHostnameCache(g_hostname, host.c_str());
  } else {
    setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  }

  const uint32_t vol = g_settingsPrefs.getUInt(kKeyVolume, SETTINGS_DEFAULT_VOLUME);

  if (vol <= SETTINGS_VOLUME_MAX &&
      settingsValidateVolume(static_cast<uint8_t>(vol))) {
    g_volume = static_cast<uint8_t>(vol);
  } else {
    g_volume = SETTINGS_DEFAULT_VOLUME;
  }

  g_welcome = g_settingsPrefs.getBool(kKeyWelcome, SETTINGS_DEFAULT_WELCOME);
  g_serialLog = g_settingsPrefs.getBool(kKeySerialLog, SETTINGS_DEFAULT_SERIAL_LOG);

  g_continuousTimeoutMin = g_settingsPrefs.getUInt(
    kKeyContTo,
    SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN
  );

  if (!settingsValidateContinuousTimeout(g_continuousTimeoutMin)) {
    g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  }

  String loading = g_settingsPrefs.getString(kKeyLoading, SETTINGS_DEFAULT_LOADING);

  if (settingsValidateLoading(loading.c_str())) {
    setLoadingCache(g_loading, loading.c_str());
  } else {
    setLoadingCache(g_loading, SETTINGS_DEFAULT_LOADING);
  }

  String accessTok =
    g_settingsPrefs.getString(kKeyAccessTok, SETTINGS_DEFAULT_ACCESS_TOKEN);

  if (settingsValidateAccessToken(accessTok.c_str())) {
    setAccessTokenCache(g_accessToken, accessTok.c_str());
  } else {
    setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  }

  String wifiSsid = g_settingsPrefs.getString(kKeyWifiSsid, "");

  if (settingsValidateWifiSsid(wifiSsid.c_str())) {
    setWifiSsidCache(g_wifiSsid, wifiSsid.c_str());
  } else {
    setWifiSsidCache(g_wifiSsid, "");
  }

  String wifiPass = g_settingsPrefs.getString(kKeyWifiPass, "");

  if (settingsValidateWifiPassword(wifiPass.c_str())) {
    setWifiPasswordCache(g_wifiPassword, wifiPass.c_str());
  } else {
    setWifiPasswordCache(g_wifiPassword, "");
  }

  uint8_t servoBlob[kServoRangesLen] = {};
  const size_t servoBytes =
    g_settingsPrefs.getBytes(kKeyServoRanges, servoBlob, kServoRangesLen);

  if (servoBytes == kServoRangesLen) {
    uint8_t loadedMin[SETTINGS_SERVO_COUNT] = {};
    uint8_t loadedMax[SETTINGS_SERVO_COUNT] = {};
    unpackServoRanges(servoBlob, loadedMin, loadedMax);

    if (settingsValidateServoRanges(loadedMin, loadedMax)) {
      memcpy(g_servoMin, loadedMin, SETTINGS_SERVO_COUNT);
      memcpy(g_servoMax, loadedMax, SETTINGS_SERVO_COUNT);
    }
  }

  String rgbOrder = g_settingsPrefs.getString(kKeyRgbOrder, SETTINGS_DEFAULT_RGB_ORDER);

  if (settingsValidateRgbOrder(rgbOrder.c_str())) {
    setRgbOrderCache(g_rgbOrder, rgbOrder.c_str());
  } else {
    setRgbOrderCache(g_rgbOrder, SETTINGS_DEFAULT_RGB_ORDER);
  }

  settingsNvsEnd();

  setHostnameCache(g_bootHostname, g_hostname);

  logSettingsSnapshot("Settings:");
}
