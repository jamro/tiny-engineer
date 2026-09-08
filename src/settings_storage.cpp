#include "settings.h"
#include "settings_internal.h"
#include "serial_log.h"
#include "servos.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

static_assert(
  SERVO_COUNT == SETTINGS_SERVO_COUNT,
  "SETTINGS_SERVO_COUNT must match SERVO_COUNT"
);

namespace {

constexpr const char* kNs = "te";
constexpr const char* kKeySleep = "sleep_m";
constexpr const char* kKeyHost = "host";
constexpr const char* kKeyVolume = "vol";
constexpr const char* kKeyWelcome = "welcome";
constexpr const char* kKeySerialLog = "serial_log";
constexpr const char* kKeyContTo = "cont_to";
constexpr const char* kKeyLoading = "loading";
constexpr const char* kKeyAccessTok = "access_tok";
constexpr const char* kKeyWifiSsid = "wifi_ssid";
constexpr const char* kKeyWifiPass = "wifi_pass";
constexpr const char* kKeyServoRanges = "sranges";
constexpr size_t kServoRangesLen = SETTINGS_SERVO_COUNT * 2;

Preferences prefs;

void fillDefaultServoRanges(uint8_t* mins, uint8_t* maxs) {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    mins[i] = static_cast<uint8_t>(SERVO_SPECS[i].min);
    maxs[i] = static_cast<uint8_t>(SERVO_SPECS[i].max);
  }
}

void packServoRanges(
  uint8_t* blob,
  const uint8_t* mins,
  const uint8_t* maxs
) {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    blob[i * 2] = mins[i];
    blob[i * 2 + 1] = maxs[i];
  }
}

void unpackServoRanges(
  const uint8_t* blob,
  uint8_t* mins,
  uint8_t* maxs
) {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    mins[i] = blob[i * 2];
    maxs[i] = blob[i * 2 + 1];
  }
}

void logServoRanges() {
  serialLogPrint(" servos=");

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (i > 0) {
      serialLogPrint(";");
    }

    serialLogPrint(g_servoMin[i]);
    serialLogPrint("-");
    serialLogPrint(g_servoMax[i]);
  }
}

}  // namespace

uint32_t g_sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
char g_hostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
char g_bootHostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
uint8_t g_volume = SETTINGS_DEFAULT_VOLUME;
bool g_welcome = SETTINGS_DEFAULT_WELCOME;
bool g_serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
uint32_t g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
char g_loading[SETTINGS_LOADING_MAX_LEN + 1] = {};
char g_accessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1] = {};
char g_wifiSsid[SETTINGS_WIFI_SSID_MAX_LEN + 1] = {};
char g_wifiPassword[SETTINGS_WIFI_PASSWORD_MAX_LEN + 1] = {};
uint8_t g_servoMin[SETTINGS_SERVO_COUNT] = {};
uint8_t g_servoMax[SETTINGS_SERVO_COUNT] = {};

void setHostnameCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_HOSTNAME_MAX_LEN);
  dest[SETTINGS_HOSTNAME_MAX_LEN] = '\0';
}

void setLoadingCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_LOADING_MAX_LEN);
  dest[SETTINGS_LOADING_MAX_LEN] = '\0';
}

void setAccessTokenCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_ACCESS_TOKEN_MAX_LEN);
  dest[SETTINGS_ACCESS_TOKEN_MAX_LEN] = '\0';
}

void setWifiSsidCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_WIFI_SSID_MAX_LEN);
  dest[SETTINGS_WIFI_SSID_MAX_LEN] = '\0';
}

void setWifiPasswordCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_WIFI_PASSWORD_MAX_LEN);
  dest[SETTINGS_WIFI_PASSWORD_MAX_LEN] = '\0';
}

void logAccessTokenState() {
  serialLogPrint(" access_token=");
  serialLogPrint(g_accessToken[0] != '\0' ? "set" : "unset");
}

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

  if (!prefs.begin(kNs, true)) {
    serialLogPrintln("Settings: NVS open failed; using defaults");
    setHostnameCache(g_bootHostname, g_hostname);
    return;
  }

  g_sleepTimeoutMin = prefs.getUInt(
    kKeySleep,
    SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN
  );

  if (!settingsValidateSleepTimeout(g_sleepTimeoutMin)) {
    g_sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
  }

  String host = prefs.getString(kKeyHost, SETTINGS_DEFAULT_HOSTNAME);

  if (settingsValidateHostname(host.c_str())) {
    setHostnameCache(g_hostname, host.c_str());
  } else {
    setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  }

  const uint32_t vol = prefs.getUInt(kKeyVolume, SETTINGS_DEFAULT_VOLUME);

  if (vol <= SETTINGS_VOLUME_MAX &&
      settingsValidateVolume(static_cast<uint8_t>(vol))) {
    g_volume = static_cast<uint8_t>(vol);
  } else {
    g_volume = SETTINGS_DEFAULT_VOLUME;
  }

  g_welcome = prefs.getBool(kKeyWelcome, SETTINGS_DEFAULT_WELCOME);
  g_serialLog = prefs.getBool(kKeySerialLog, SETTINGS_DEFAULT_SERIAL_LOG);

  g_continuousTimeoutMin = prefs.getUInt(
    kKeyContTo,
    SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN
  );

  if (!settingsValidateContinuousTimeout(g_continuousTimeoutMin)) {
    g_continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  }

  String loading = prefs.getString(kKeyLoading, SETTINGS_DEFAULT_LOADING);

  if (settingsValidateLoading(loading.c_str())) {
    setLoadingCache(g_loading, loading.c_str());
  } else {
    setLoadingCache(g_loading, SETTINGS_DEFAULT_LOADING);
  }

  String accessTok =
    prefs.getString(kKeyAccessTok, SETTINGS_DEFAULT_ACCESS_TOKEN);

  if (settingsValidateAccessToken(accessTok.c_str())) {
    setAccessTokenCache(g_accessToken, accessTok.c_str());
  } else {
    setAccessTokenCache(g_accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN);
  }

  String wifiSsid = prefs.getString(kKeyWifiSsid, "");

  if (settingsValidateWifiSsid(wifiSsid.c_str())) {
    setWifiSsidCache(g_wifiSsid, wifiSsid.c_str());
  } else {
    setWifiSsidCache(g_wifiSsid, "");
  }

  String wifiPass = prefs.getString(kKeyWifiPass, "");

  if (settingsValidateWifiPassword(wifiPass.c_str())) {
    setWifiPasswordCache(g_wifiPassword, wifiPass.c_str());
  } else {
    setWifiPasswordCache(g_wifiPassword, "");
  }

  uint8_t servoBlob[kServoRangesLen] = {};
  const size_t servoBytes =
    prefs.getBytes(kKeyServoRanges, servoBlob, kServoRangesLen);

  if (servoBytes == kServoRangesLen) {
    uint8_t loadedMin[SETTINGS_SERVO_COUNT] = {};
    uint8_t loadedMax[SETTINGS_SERVO_COUNT] = {};
    unpackServoRanges(servoBlob, loadedMin, loadedMax);

    if (settingsValidateServoRanges(loadedMin, loadedMax)) {
      memcpy(g_servoMin, loadedMin, SETTINGS_SERVO_COUNT);
      memcpy(g_servoMax, loadedMax, SETTINGS_SERVO_COUNT);
    }
  }

  prefs.end();

  setHostnameCache(g_bootHostname, g_hostname);

  serialLogPrint("Settings: sleep_timeout=");
  serialLogPrint(g_sleepTimeoutMin);
  serialLogPrint("min hostname=");
  serialLogPrint(g_hostname);
  serialLogPrint(" volume=");
  serialLogPrint(g_volume);
  serialLogPrint(" welcome=");
  serialLogPrint(g_welcome ? "on" : "off");
  serialLogPrint(" serial_log=");
  serialLogPrint(g_serialLog ? "on" : "off");
  serialLogPrint(" continuous_timeout=");
  serialLogPrint(g_continuousTimeoutMin);
  serialLogPrint("min loading=");
  serialLogPrint(g_loading);
  logAccessTokenState();
  serialLogPrint(" wifi=");
  serialLogPrint(g_wifiSsid[0] != '\0' ? "configured" : "unset");
  logServoRanges();
  serialLogPrintln();
}

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
      servoMaxs == nullptr) {
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

  if (!prefs.begin(kNs, false)) {
    serialLogPrintln("Settings: NVS write open failed");
    return false;
  }

  uint8_t servoBlob[kServoRangesLen];
  packServoRanges(servoBlob, nextServoMin, nextServoMax);

  prefs.putUInt(kKeySleep, nextSleep);
  prefs.putString(kKeyHost, nextHost);
  prefs.putUInt(kKeyVolume, nextVolume);
  prefs.putBool(kKeyWelcome, nextWelcome);
  prefs.putBool(kKeySerialLog, nextSerialLog);
  prefs.putUInt(kKeyContTo, nextContTo);
  prefs.putString(kKeyLoading, nextLoading);
  prefs.putString(kKeyAccessTok, nextAccessToken);
  prefs.putString(kKeyWifiSsid, nextWifiSsid);
  prefs.putString(kKeyWifiPass, nextWifiPassword);
  prefs.putBytes(kKeyServoRanges, servoBlob, kServoRangesLen);
  prefs.end();

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

  if (rebootRequired != nullptr &&
      strcmp(g_hostname, g_bootHostname) != 0) {
    *rebootRequired = true;
  }

  serialLogPrint("Settings saved: sleep_timeout=");
  serialLogPrint(g_sleepTimeoutMin);
  serialLogPrint("min hostname=");
  serialLogPrint(g_hostname);
  serialLogPrint(" volume=");
  serialLogPrint(g_volume);
  serialLogPrint(" welcome=");
  serialLogPrint(g_welcome ? "on" : "off");
  serialLogPrint(" serial_log=");
  serialLogPrint(g_serialLog ? "on" : "off");
  serialLogPrint(" continuous_timeout=");
  serialLogPrint(g_continuousTimeoutMin);
  serialLogPrint("min loading=");
  serialLogPrint(g_loading);
  logAccessTokenState();
  serialLogPrint(" wifi=");
  serialLogPrint(g_wifiSsid[0] != '\0' ? "configured" : "unset");
  logServoRanges();
  serialLogPrintln();

  return true;
}

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
  // Keep g_servoMin / g_servoMax; write them back after prefs.clear().

  if (!prefs.begin(kNs, false)) {
    serialLogPrintln("Settings: factory reset NVS open failed");
    return false;
  }

  prefs.clear();
  prefs.putUInt(kKeySleep, g_sleepTimeoutMin);
  prefs.putString(kKeyHost, g_hostname);
  prefs.putUInt(kKeyVolume, g_volume);
  prefs.putBool(kKeyWelcome, g_welcome);
  prefs.putBool(kKeySerialLog, g_serialLog);
  prefs.putUInt(kKeyContTo, g_continuousTimeoutMin);
  prefs.putString(kKeyLoading, g_loading);
  prefs.putString(kKeyAccessTok, g_accessToken);
  prefs.putString(kKeyWifiSsid, g_wifiSsid);
  prefs.putString(kKeyWifiPass, g_wifiPassword);
  uint8_t servoBlob[kServoRangesLen];
  packServoRanges(servoBlob, g_servoMin, g_servoMax);
  prefs.putBytes(kKeyServoRanges, servoBlob, kServoRangesLen);
  prefs.end();

  serialLogPrint("Settings factory reset: sleep_timeout=");
  serialLogPrint(g_sleepTimeoutMin);
  serialLogPrint("min hostname=");
  serialLogPrint(g_hostname);
  serialLogPrint(" volume=");
  serialLogPrint(g_volume);
  serialLogPrint(" welcome=");
  serialLogPrint(g_welcome ? "on" : "off");
  serialLogPrint(" serial_log=");
  serialLogPrint(g_serialLog ? "on" : "off");
  serialLogPrint(" continuous_timeout=");
  serialLogPrint(g_continuousTimeoutMin);
  serialLogPrint("min loading=");
  serialLogPrint(g_loading);
  logAccessTokenState();
  serialLogPrint(" wifi=");
  serialLogPrint("unset");
  logServoRanges();
  serialLogPrintln();

  return true;
}
