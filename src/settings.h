#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint32_t SETTINGS_DEFAULT_SLEEP_TIMEOUT_S = 60;
constexpr const char* SETTINGS_DEFAULT_HOSTNAME = "tiny-engineer";
constexpr uint8_t SETTINGS_DEFAULT_VOLUME = 70;

constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MIN_S = 5;
constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MAX_S = 3600;
constexpr size_t SETTINGS_HOSTNAME_MAX_LEN = 31;
constexpr uint8_t SETTINGS_VOLUME_MIN = 0;
constexpr uint8_t SETTINGS_VOLUME_MAX = 100;

void initSettings();

uint32_t settingsSleepTimeoutS();
uint32_t settingsSleepTimeoutMs();
const char* settingsHostname();
uint8_t settingsVolume();

// Hostname used at boot for Wi-Fi/mDNS (frozen after initSettings).
const char* settingsBootHostname();

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutS);
bool settingsValidateHostname(const char* hostname);
bool settingsValidateVolume(uint8_t volume);

// Updates RAM cache and NVS for any non-null args. Returns false on validation
// failure (no write). If hostname is set and differs from boot hostname,
// *rebootRequired is set true when non-null.
bool saveSettings(
  const uint32_t* sleepTimeoutS,
  const char* hostname,
  const uint8_t* volume,
  bool* rebootRequired
);
