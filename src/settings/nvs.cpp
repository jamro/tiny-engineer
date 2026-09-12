#include "settings/nvs.h"

#include "serial_log.h"
#include "servos.h"

Preferences g_settingsPrefs;

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

bool settingsNvsBeginRead() {
  return g_settingsPrefs.begin(kNs, true);
}

bool settingsNvsBeginWrite() {
  return g_settingsPrefs.begin(kNs, false);
}

void settingsNvsEnd() {
  g_settingsPrefs.end();
}

void settingsNvsClear() {
  g_settingsPrefs.clear();
}

namespace {

void putAll(
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
  const char* rgbOrder,
  bool oledRotate180
) {
  uint8_t servoBlob[kServoRangesLen];
  packServoRanges(servoBlob, servoMins, servoMaxs);

  g_settingsPrefs.putUInt(kKeySleep, sleepTimeoutMin);
  g_settingsPrefs.putString(kKeyHost, hostname);
  g_settingsPrefs.putUInt(kKeyVolume, volume);
  g_settingsPrefs.putBool(kKeyWelcome, welcome);
  g_settingsPrefs.putBool(kKeySerialLog, serialLog);
  g_settingsPrefs.putUInt(kKeyContTo, continuousTimeoutMin);
  g_settingsPrefs.putString(kKeyLoading, loading);
  g_settingsPrefs.putString(kKeyAccessTok, accessToken);
  g_settingsPrefs.putString(kKeyWifiSsid, wifiSsid);
  g_settingsPrefs.putString(kKeyWifiPass, wifiPassword);
  g_settingsPrefs.putBytes(kKeyServoRanges, servoBlob, kServoRangesLen);
  g_settingsPrefs.putString(kKeyRgbOrder, rgbOrder);
  g_settingsPrefs.putBool(kKeyOledRot, oledRotate180);
}

}  // namespace

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
  const char* rgbOrder,
  bool oledRotate180
) {
  if (!settingsNvsBeginWrite()) {
    serialLogPrintln("Settings: NVS write open failed");
    return false;
  }

  putAll(
    sleepTimeoutMin,
    hostname,
    volume,
    welcome,
    serialLog,
    continuousTimeoutMin,
    loading,
    accessToken,
    wifiSsid,
    wifiPassword,
    servoMins,
    servoMaxs,
    rgbOrder,
    oledRotate180
  );
  settingsNvsEnd();
  return true;
}

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
  const char* rgbOrder,
  bool oledRotate180
) {
  putAll(
    sleepTimeoutMin,
    hostname,
    volume,
    welcome,
    serialLog,
    continuousTimeoutMin,
    loading,
    accessToken,
    wifiSsid,
    wifiPassword,
    servoMins,
    servoMaxs,
    rgbOrder,
    oledRotate180
  );
}
