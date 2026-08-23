#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

namespace {

constexpr const char* kNs = "te";
constexpr const char* kKeySleep = "sleep_s";
constexpr const char* kKeyHost = "host";
constexpr const char* kKeyVolume = "vol";
constexpr const char* kKeyWelcome = "welcome";

Preferences prefs;

uint32_t g_sleepTimeoutS = SETTINGS_DEFAULT_SLEEP_TIMEOUT_S;
char g_hostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
char g_bootHostname[SETTINGS_HOSTNAME_MAX_LEN + 1] = {};
uint8_t g_volume = SETTINGS_DEFAULT_VOLUME;
bool g_welcome = SETTINGS_DEFAULT_WELCOME;

void setHostnameCache(char* dest, const char* src) {
  strncpy(dest, src, SETTINGS_HOSTNAME_MAX_LEN);
  dest[SETTINGS_HOSTNAME_MAX_LEN] = '\0';
}

}  // namespace

void initSettings() {
  setHostnameCache(g_hostname, SETTINGS_DEFAULT_HOSTNAME);
  g_volume = SETTINGS_DEFAULT_VOLUME;
  g_welcome = SETTINGS_DEFAULT_WELCOME;

  if (!prefs.begin(kNs, true)) {
    Serial.println("Settings: NVS open failed; using defaults");
    setHostnameCache(g_bootHostname, g_hostname);
    return;
  }

  g_sleepTimeoutS = prefs.getUInt(
    kKeySleep,
    SETTINGS_DEFAULT_SLEEP_TIMEOUT_S
  );

  if (!settingsValidateSleepTimeout(g_sleepTimeoutS)) {
    g_sleepTimeoutS = SETTINGS_DEFAULT_SLEEP_TIMEOUT_S;
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

  prefs.end();

  setHostnameCache(g_bootHostname, g_hostname);

  Serial.print("Settings: sleep_timeout=");
  Serial.print(g_sleepTimeoutS);
  Serial.print("s hostname=");
  Serial.print(g_hostname);
  Serial.print(" volume=");
  Serial.print(g_volume);
  Serial.print(" welcome=");
  Serial.println(g_welcome ? "on" : "off");
}

uint32_t settingsSleepTimeoutS() {
  return g_sleepTimeoutS;
}

uint32_t settingsSleepTimeoutMs() {
  return g_sleepTimeoutS * 1000UL;
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

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutS) {
  return sleepTimeoutS >= SETTINGS_SLEEP_TIMEOUT_MIN_S &&
         sleepTimeoutS <= SETTINGS_SLEEP_TIMEOUT_MAX_S;
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

bool saveSettings(
  const uint32_t* sleepTimeoutS,
  const char* hostname,
  const uint8_t* volume,
  const bool* welcome,
  bool* rebootRequired
) {
  if (rebootRequired != nullptr) {
    *rebootRequired = false;
  }

  if (sleepTimeoutS == nullptr &&
      hostname == nullptr &&
      volume == nullptr &&
      welcome == nullptr) {
    return false;
  }

  uint32_t nextSleep = g_sleepTimeoutS;
  char nextHost[SETTINGS_HOSTNAME_MAX_LEN + 1];
  setHostnameCache(nextHost, g_hostname);
  uint8_t nextVolume = g_volume;
  bool nextWelcome = g_welcome;

  if (sleepTimeoutS != nullptr) {
    if (!settingsValidateSleepTimeout(*sleepTimeoutS)) {
      return false;
    }

    nextSleep = *sleepTimeoutS;
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

  if (!prefs.begin(kNs, false)) {
    Serial.println("Settings: NVS write open failed");
    return false;
  }

  prefs.putUInt(kKeySleep, nextSleep);
  prefs.putString(kKeyHost, nextHost);
  prefs.putUInt(kKeyVolume, nextVolume);
  prefs.putBool(kKeyWelcome, nextWelcome);
  prefs.end();

  g_sleepTimeoutS = nextSleep;
  setHostnameCache(g_hostname, nextHost);
  g_volume = nextVolume;
  g_welcome = nextWelcome;

  if (rebootRequired != nullptr &&
      strcmp(g_hostname, g_bootHostname) != 0) {
    *rebootRequired = true;
  }

  Serial.print("Settings saved: sleep_timeout=");
  Serial.print(g_sleepTimeoutS);
  Serial.print("s hostname=");
  Serial.print(g_hostname);
  Serial.print(" volume=");
  Serial.print(g_volume);
  Serial.print(" welcome=");
  Serial.println(g_welcome ? "on" : "off");

  return true;
}
