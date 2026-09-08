#include "settings/internal.h"
#include "settings/nvs.h"
#include "serial_log.h"
#include "settings/settings.h"

#include <cstring>

bool factoryResetSettings(bool* rebootRequired) {
  const bool hostReboot =
    strcmp(g_bootHostname, SETTINGS_DEFAULT_HOSTNAME) != 0;
  const bool loadingReboot =
    strcmp(g_loading, SETTINGS_DEFAULT_LOADING) != 0;
  const bool wifiReboot = g_wifiSsid[0] != '\0';

  if (rebootRequired != nullptr) {
    *rebootRequired = hostReboot || loadingReboot || wifiReboot;
  }

  g_sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
  setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  g_volume = SETTINGS_DEFAULT_VOLUME;
  g_welcome = SETTINGS_DEFAULT_WELCOME;
  g_serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
  g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  setLoadingCache(g_loading, SETTINGS_DEFAULT_LOADING);
  setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  setWifiSsidCache(g_wifiSsid, "");
  setWifiPasswordCache(g_wifiPassword, "");
  // Keep g_servoMin / g_servoMax and g_rgbOrder; write them back after
  // prefs.clear().

  if (!settingsNvsBeginWrite()) {
    serialLogPrintln("Settings: factory reset NVS open failed");
    return false;
  }

  settingsNvsClear();
  settingsNvsPutAll(
    g_sleepTimeoutMin,
    g_hostname,
    g_volume,
    g_welcome,
    g_serialLog,
    g_continuousTimeoutMin,
    g_loading,
    g_accessToken,
    g_wifiSsid,
    g_wifiPassword,
    g_servoMin,
    g_servoMax,
    g_rgbOrder
  );
  settingsNvsEnd();

  logSettingsSnapshot("Settings factory reset:");
  return true;
}
