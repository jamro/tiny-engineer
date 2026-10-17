#include "settings/nvs.h"

#include "serial_log.h"
#include "settings/values.h"

Preferences g_settingsPrefs;

namespace {

// Stored as min,max pairs, one per servo.
constexpr size_t kServoRangesLen = SETTINGS_SERVO_COUNT * 2;

template <size_t N>
void loadText(char (&dest)[N], const char* key, bool (*valid)(const char*)) {
  // isKey first: Preferences getString logs ERROR when the key is missing.
  if (!g_settingsPrefs.isKey(key)) {
    return;
  }

  const String value = g_settingsPrefs.getString(key, dest);

  if (valid(value.c_str())) {
    strlcpy(dest, value.c_str(), sizeof(dest));
  }
}

void loadMinutes(uint32_t& dest, const char* key, bool (*valid)(uint32_t)) {
  if (!g_settingsPrefs.isKey(key)) {
    return;
  }

  const uint32_t value = g_settingsPrefs.getUInt(key, dest);

  if (valid(value)) {
    dest = value;
  }
}

}  // namespace

void SettingsValues::load() {
  loadMinutes(sleepTimeoutMin, kKeySleep, settingsValidateSleepTimeout);
  loadText(hostname, kKeyHost, settingsValidateHostname);

  if (g_settingsPrefs.isKey(kKeyVolume)) {
    const uint32_t storedVolume = g_settingsPrefs.getUInt(kKeyVolume, volume);

    if (storedVolume <= UINT8_MAX && settingsValidateVolume(storedVolume)) {
      volume = static_cast<uint8_t>(storedVolume);
    }
  }

  if (g_settingsPrefs.isKey(kKeyWelcome)) {
    welcome = g_settingsPrefs.getBool(kKeyWelcome, welcome);
  }

  if (g_settingsPrefs.isKey(kKeySerialLog)) {
    serialLog = g_settingsPrefs.getBool(kKeySerialLog, serialLog);
  }

  loadMinutes(continuousTimeoutMin, kKeyContTo, settingsValidateContinuousTimeout);
  loadText(loading, kKeyLoading, settingsValidateLoading);
  loadText(eyesStyle, kKeyEyesStyle, settingsValidateEyesStyle);
  loadText(accessToken, kKeyAccessTok, settingsValidateAccessToken);
  loadText(linkMode, kKeyLinkMode, settingsValidateLinkMode);

  if (g_settingsPrefs.isKey(kKeyServoRanges)) {
    uint8_t blob[kServoRangesLen] = {};

    if (g_settingsPrefs.getBytes(kKeyServoRanges, blob, kServoRangesLen) ==
        kServoRangesLen) {
      ServoRanges stored{};

      for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
        stored.mins[i] = blob[i * 2];
        stored.maxs[i] = blob[i * 2 + 1];
      }

      if (stored.valid()) {
        servoRanges = stored;
      }
    }
  }

  loadText(rgbOrder, kKeyRgbOrder, settingsValidateRgbOrder);

  if (g_settingsPrefs.isKey(kKeyOledRot)) {
    oledRotate180 = g_settingsPrefs.getBool(kKeyOledRot, oledRotate180);
  }
}

namespace {

bool openForWrite() {
  if (g_settingsPrefs.begin(kNs, false)) {
    return true;
  }

  serialLogPrintln("Settings: NVS write open failed");
  return false;
}

// Preferences commits every put on its own, so only keys whose value differs
// from `before` are written, unless `all` asks for every key.
bool writeValues(const SettingsValues& values, const SettingsValues& before, bool all) {
  bool ok = true;

  auto putUInt = [&](const char* key, uint32_t value, uint32_t old) {
    if (all || value != old) {
      ok &= g_settingsPrefs.putUInt(key, value) == sizeof(uint32_t);
    }
  };
  auto putBool = [&](const char* key, bool value, bool old) {
    if (all || value != old) {
      ok &= g_settingsPrefs.putBool(key, value) == sizeof(uint8_t);
    }
  };
  auto putText = [&](const char* key, const char* value, const char* old) {
    if (all || strcmp(value, old) != 0) {
      ok &= g_settingsPrefs.putString(key, value) == strlen(value);
    }
  };

  putUInt(kKeySleep, values.sleepTimeoutMin, before.sleepTimeoutMin);
  putText(kKeyHost, values.hostname, before.hostname);
  putUInt(kKeyVolume, values.volume, before.volume);
  putBool(kKeyWelcome, values.welcome, before.welcome);
  putBool(kKeySerialLog, values.serialLog, before.serialLog);
  putUInt(kKeyContTo, values.continuousTimeoutMin, before.continuousTimeoutMin);
  putText(kKeyLoading, values.loading, before.loading);
  putText(kKeyEyesStyle, values.eyesStyle, before.eyesStyle);
  putText(kKeyAccessTok, values.accessToken, before.accessToken);
  putText(kKeyLinkMode, values.linkMode, before.linkMode);
  putText(kKeyRgbOrder, values.rgbOrder, before.rgbOrder);
  putBool(kKeyOledRot, values.oledRotate180, before.oledRotate180);

  if (all || memcmp(&values.servoRanges, &before.servoRanges, sizeof(ServoRanges)) != 0) {
    uint8_t blob[kServoRangesLen];

    for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
      blob[i * 2] = values.servoRanges.mins[i];
      blob[i * 2 + 1] = values.servoRanges.maxs[i];
    }

    ok &= g_settingsPrefs.putBytes(kKeyServoRanges, blob, kServoRangesLen) == kServoRangesLen;
  }

  return ok;
}

}  // namespace

bool SettingsValues::save(const SettingsValues& saved) const {
  if (!openForWrite()) {
    return false;
  }

  const bool ok = writeValues(*this, saved, false);
  g_settingsPrefs.end();
  return ok;
}

bool SettingsValues::replaceAll() const {
  if (!openForWrite()) {
    return false;
  }

  g_settingsPrefs.clear();
  const bool ok = writeValues(*this, *this, true);
  g_settingsPrefs.end();
  return ok;
}

bool saveSettings(const SettingsPatch& patch) {
  if (patch.invalidField() != nullptr) {
    return false;
  }

  SettingsValues next = g_settings;
  next.apply(patch);

  if (!next.save(g_settings)) {
    return false;
  }

  g_settings = next;
  g_settings.log("Settings saved:");
  return true;
}

bool saveWifiNetworks(const WifiNetworkList& networks) {
  if (!openForWrite()) {
    return false;
  }

  const size_t length = networks.count() * sizeof(WifiNetwork);
  // Preferences writes nothing for an empty blob, so an empty list removes the key.
  const bool written = length == 0
    ? !g_settingsPrefs.isKey(kKeyWifiNets) || g_settingsPrefs.remove(kKeyWifiNets)
    : g_settingsPrefs.putBytes(kKeyWifiNets, networks.data(), length) == length;

  g_settingsPrefs.end();

  if (!written) {
    serialLogPrintln("Settings: WiFi networks write failed");
    return false;
  }

  g_wifiNetworks = networks;
  g_wifiChangedSinceBoot = true;
  g_settings.log("Settings saved:");
  return true;
}

bool factoryResetSettings() {
  // The hardware calibration survives a reset.
  SettingsValues next = SettingsValues::defaults();
  next.servoRanges = g_settings.servoRanges;
  strlcpy(next.rgbOrder, g_settings.rgbOrder, sizeof(next.rgbOrder));
  next.oledRotate180 = g_settings.oledRotate180;

  if (!next.replaceAll()) {
    serialLogPrintln("Settings: factory reset failed");
    return false;
  }

  g_settings = next;
  g_wifiChangedSinceBoot |= g_wifiNetworks.count() > 0;
  g_wifiNetworks.clear();
  g_settings.log("Settings factory reset:");
  return true;
}

bool settingsRebootRequired() {
  return strcmp(g_settings.hostname, g_bootSettings.hostname) != 0 ||
         strcmp(g_settings.loading, g_bootSettings.loading) != 0 ||
         g_wifiChangedSinceBoot;
}
