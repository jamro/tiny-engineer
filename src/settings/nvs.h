#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "settings/settings.h"

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
constexpr const char* kKeyRgbOrder = "rgb_ord";
constexpr size_t kServoRangesLen = SETTINGS_SERVO_COUNT * 2;

extern Preferences g_settingsPrefs;

void fillDefaultServoRanges(uint8_t* mins, uint8_t* maxs);
void packServoRanges(
  uint8_t* blob,
  const uint8_t* mins,
  const uint8_t* maxs
);
void unpackServoRanges(
  const uint8_t* blob,
  uint8_t* mins,
  uint8_t* maxs
);

bool settingsNvsBeginRead();
bool settingsNvsBeginWrite();
void settingsNvsEnd();
void settingsNvsClear();

void settingsNvsPutAll(
  uint32_t sleepTimeoutMin,
  const char* hostname,
  uint8_t volume,
  bool welcome,
  bool serialLog,
  uint32_t continuousTimeoutMin,
  const char* loading,
  const char* accessToken,
  const char* wifiSsid,
  const char* wifiPassword,
  const uint8_t* servoMins,
  const uint8_t* servoMaxs,
  const char* rgbOrder
);

bool writeAllToNvs(
  uint32_t sleepTimeoutMin,
  const char* hostname,
  uint8_t volume,
  bool welcome,
  bool serialLog,
  uint32_t continuousTimeoutMin,
  const char* loading,
  const char* accessToken,
  const char* wifiSsid,
  const char* wifiPassword,
  const uint8_t* servoMins,
  const uint8_t* servoMaxs,
  const char* rgbOrder
);
