#include "actions/migrate.h"
#include "serial/link.h"
#include "serial_log.h"
#include "settings/nvs.h"
#include "settings/settings.h"
#include "settings/values.h"

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
  if (!g_settingsPrefs.begin(kNs, false)) {
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

  g_settingsPrefs.end();
}

// Older firmware stored servo_hold here. Idle/sleep always limp now.
constexpr const char* kLegacyServoHold = "srv_hold";

void dropLegacyServoHold() {
  if (g_settingsPrefs.isKey(kLegacyServoHold)) {
    g_settingsPrefs.remove(kLegacyServoHold);
  }
}

void loadActionsBlob() {
  if (!g_settingsPrefs.isKey(kKeyActions)) {
    return;
  }

  uint8_t blob[sizeof(ActionEntry) * ACTIONS_MAX] = {};
  const size_t length =
    g_settingsPrefs.getBytes(kKeyActions, blob, sizeof(blob));
  g_actions.load(blob, length);
}

void clearLegacyButtonSoundKeys() {
  g_settingsPrefs.remove(kKeyBtnSndDn);
  g_settingsPrefs.remove(kKeyBtnSndUp);
  g_settingsPrefs.remove(kKeyBtnSndClk);
  g_settingsPrefs.remove(kKeyBtnSndDbl);
  g_settingsPrefs.remove(kKeyBtnSndLng);
}

void migrateLegacyButtonSounds() {
  const bool hadLegacyKeys =
    g_settingsPrefs.isKey(kKeyBtnSndDn) ||
    g_settingsPrefs.isKey(kKeyBtnSndUp) ||
    g_settingsPrefs.isKey(kKeyBtnSndClk) ||
    g_settingsPrefs.isKey(kKeyBtnSndDbl) ||
    g_settingsPrefs.isKey(kKeyBtnSndLng);

  if (!hadLegacyKeys) {
    return;
  }

  ActionList next = g_actions;
  const bool migrated = actionsMigrateLegacyButtonSounds(
    next,
    g_settings.pushButtonSoundDown,
    g_settings.pushButtonSoundUp,
    g_settings.pushButtonSoundClick,
    g_settings.pushButtonSoundDoubleClick,
    g_settings.pushButtonSoundLongClick
  );

  if (migrated) {
    const size_t length = next.count() * sizeof(ActionEntry);
    if (g_settingsPrefs.putBytes(kKeyActions, next.data(), length) != length) {
      return;
    }
    g_actions = next;
  }

  clearLegacyButtonSoundKeys();
}

}  // namespace

void initSettings() {
  g_settings = SettingsValues::defaults();
  g_wifiNetworks.clear();
  g_actions.clear();
  bool legacyWifi = false;

  // RW so a missing namespace is created without Preferences NOT_FOUND ERROR.
  // Boot reads settings and drops the retired srv_hold key; saves use openForWrite.
  if (g_settingsPrefs.begin(kNs, false)) {
    g_settings.load();
    legacyWifi = loadWifiNetworks();
    loadActionsBlob();
    migrateLegacyButtonSounds();
    dropLegacyServoHold();
    g_settingsPrefs.end();
  } else {
    serialLogPrintln("Settings: NVS open failed; using defaults");
  }

  g_bootSettings = g_settings;

  if (legacyWifi) {
    migrateLegacyWifi();
  }

  // Older firmware had Wi-Fi nets but no link_mode; treat as Wi-Fi mode.
  if (g_settings.linkMode[0] == '\0' && g_wifiNetworks.count() > 0) {
    SettingsPatch patch;
    patch.linkMode = "wifi";
    saveSettings(patch);
  }

  // Framed serial API (and LOG frames) start after NVS is loaded.
  serialLinkBegin();
  g_settings.log("Settings:");
}
