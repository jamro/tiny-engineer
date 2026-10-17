#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "settings/settings.h"
#include "settings/wifi_network_list.h"

// Every saved setting except the WiFi networks, which have their own NVS key.
struct SettingsValues {
  uint32_t sleepTimeoutMin;
  char hostname[SETTINGS_HOSTNAME_MAX_LEN + 1];
  uint8_t volume;
  bool welcome;
  bool serialLog;
  uint32_t continuousTimeoutMin;
  char loading[SETTINGS_LOADING_MAX_LEN + 1];
  char eyesStyle[SETTINGS_EYES_STYLE_MAX_LEN + 1];
  char accessToken[SETTINGS_ACCESS_TOKEN_MAX_LEN + 1];
  char linkMode[SETTINGS_LINK_MODE_MAX_LEN + 1];
  ServoRanges servoRanges;
  char rgbOrder[SETTINGS_RGB_ORDER_MAX_LEN + 1];
  bool oledRotate180;

  static SettingsValues defaults();

  // The patch must be valid (SettingsPatch::invalidField() == nullptr).
  void apply(const SettingsPatch& patch);

  // Reads NVS over the current values; keys that are missing or invalid keep theirs.
  void load();

  // Writes the keys that differ from `saved`, the values NVS holds now.
  bool save(const SettingsValues& saved) const;

  // Erases the namespace, WiFi networks included, then writes every key.
  bool replaceAll() const;

  void log(const char* prefix) const;
};

extern SettingsValues g_settings;
extern WifiNetworkList g_wifiNetworks;
// Hostname and loading screen are read at boot only; a change waits for a reboot.
extern SettingsValues g_bootSettings;
extern bool g_wifiChangedSinceBoot;
