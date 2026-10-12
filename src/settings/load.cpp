#include "settings/internal.h"
#include "settings/nvs.h"
#include "serial_log.h"
#include "settings/settings.h"

#include <Arduino.h>
#include <cstring>

namespace {

// True when the networks came from the single-network keys of older firmware.
bool loadWifiNetworks() {
  if (g_settingsPrefs.isKey(kKeyWifiNets)) {
    uint8_t blob[sizeof(WifiNetwork) * SETTINGS_WIFI_NETWORK_MAX] = {};
    const size_t length =
      g_settingsPrefs.getBytes(kKeyWifiNets, blob, sizeof(blob));
    g_wifiNetworks.load(blob, length);
    return false;
  }

  if (!g_settingsPrefs.isKey(kKeyWifiSsid)) {
    return false;
  }

  const String ssid = g_settingsPrefs.getString(kKeyWifiSsid, "");
  const String password = g_settingsPrefs.getString(kKeyWifiPass, "");
  g_wifiNetworks.add(ssid.c_str(), password.c_str());
  return true;
}

void migrateLegacyWifi() {
  if (!settingsNvsBeginWrite()) {
    return;
  }

  // An invalid legacy SSID loads as an empty list; its keys go too.
  const size_t length = g_wifiNetworks.count() * sizeof(WifiNetwork);
  const bool moved =
    length == 0 ||
    g_settingsPrefs.putBytes(kKeyWifiNets, g_wifiNetworks.data(), length) ==
      length;

  if (moved) {
    g_settingsPrefs.remove(kKeyWifiSsid);
    g_settingsPrefs.remove(kKeyWifiPass);
  }

  settingsNvsEnd();
}

}  // namespace

void initSettings() {
  setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  g_volume = SETTINGS_DEFAULT_VOLUME;
  g_welcome = SETTINGS_DEFAULT_WELCOME;
  g_serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
  g_servoHold = SETTINGS_DEFAULT_SERVO_HOLD;
  g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  setLoadingCache(g_loading, SETTINGS_DEFAULT_LOADING);
  setEyesStyleCache(g_eyesStyle, SETTINGS_DEFAULT_EYES_STYLE);
  setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  g_wifiNetworks.clear();
  fillDefaultServoRanges(g_servoMin, g_servoMax);
  setRgbOrderCache(g_rgbOrder, SETTINGS_DEFAULT_RGB_ORDER);
  g_oledRotate180 = SETTINGS_DEFAULT_OLED_ROTATE_180;

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
  g_servoHold = g_settingsPrefs.getBool(kKeyServoHold, SETTINGS_DEFAULT_SERVO_HOLD);

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

  String eyesStyle =
    g_settingsPrefs.getString(kKeyEyesStyle, SETTINGS_DEFAULT_EYES_STYLE);

  if (settingsValidateEyesStyle(eyesStyle.c_str())) {
    setEyesStyleCache(g_eyesStyle, eyesStyle.c_str());
  } else {
    setEyesStyleCache(g_eyesStyle, SETTINGS_DEFAULT_EYES_STYLE);
  }

  String accessTok =
    g_settingsPrefs.getString(kKeyAccessTok, SETTINGS_DEFAULT_ACCESS_TOKEN);

  if (settingsValidateAccessToken(accessTok.c_str())) {
    setAccessTokenCache(g_accessToken, accessTok.c_str());
  } else {
    setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  }

  const bool legacyWifi = loadWifiNetworks();

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

  g_oledRotate180 = g_settingsPrefs.getBool(
    kKeyOledRot,
    SETTINGS_DEFAULT_OLED_ROTATE_180
  );

  settingsNvsEnd();

  setHostnameCache(g_bootHostname, g_hostname);

  if (legacyWifi) {
    migrateLegacyWifi();
  }

  logSettingsSnapshot("Settings:");
}
