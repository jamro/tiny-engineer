#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint32_t SETTINGS_DEFAULT_SLEEP_TIMEOUT_S = 60;
constexpr const char* SETTINGS_DEFAULT_HOSTNAME = "tiny-engineer";

constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MIN_S = 5;
constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MAX_S = 3600;
constexpr size_t SETTINGS_HOSTNAME_MAX_LEN = 31;

void initSettings();

uint32_t settingsSleepTimeoutS();
uint32_t settingsSleepTimeoutMs();
const char* settingsHostname();

// Hostname used at boot for Wi-Fi/mDNS (frozen after initSettings).
const char* settingsBootHostname();

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutS);
bool settingsValidateHostname(const char* hostname);

// Updates RAM cache and NVS for any non-null args. Returns false on validation
// failure (no write). If hostname is set and differs from boot hostname,
// *rebootRequired is set true when non-null.
bool saveSettings(
  const uint32_t* sleepTimeoutS,
  const char* hostname,
  bool* rebootRequired
);
