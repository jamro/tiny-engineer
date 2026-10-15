#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

class WifiNetworkList;

constexpr uint32_t SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN = 10;
constexpr const char* SETTINGS_DEFAULT_HOSTNAME = "tiny-engineer";
constexpr uint8_t SETTINGS_DEFAULT_VOLUME = 70;
constexpr bool SETTINGS_DEFAULT_WELCOME = true;
constexpr bool SETTINGS_DEFAULT_SERIAL_LOG = false;
constexpr uint32_t SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN = 5;
constexpr const char* SETTINGS_DEFAULT_LOADING = "progress";
constexpr const char* SETTINGS_DEFAULT_EYES_STYLE = "classic";
constexpr const char* SETTINGS_DEFAULT_ACCESS_TOKEN = "";

constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MIN_MIN = 1;
constexpr uint32_t SETTINGS_SLEEP_TIMEOUT_MAX_MIN = 1440;
constexpr size_t SETTINGS_HOSTNAME_MAX_LEN = 31;
constexpr size_t SETTINGS_LOADING_MAX_LEN = 15;
constexpr size_t SETTINGS_EYES_STYLE_MAX_LEN = 15;
constexpr size_t SETTINGS_ACCESS_TOKEN_MAX_LEN = 64;
constexpr size_t SETTINGS_WIFI_SSID_MAX_LEN = 32;
constexpr size_t SETTINGS_WIFI_PASSWORD_MAX_LEN = 63;
constexpr size_t SETTINGS_WIFI_NETWORK_MAX = 5;
constexpr uint8_t SETTINGS_VOLUME_MAX = 100;
constexpr uint32_t SETTINGS_CONTINUOUS_TIMEOUT_MIN_MIN = 1;
constexpr uint32_t SETTINGS_CONTINUOUS_TIMEOUT_MAX_MIN = 1440;
constexpr size_t SETTINGS_SERVO_COUNT = 5;
constexpr uint8_t SETTINGS_SERVO_ANGLE_MAX = 180;
constexpr size_t SETTINGS_RGB_ORDER_MAX_LEN = 3;
constexpr const char* SETTINGS_DEFAULT_RGB_ORDER = "GRB";
constexpr bool SETTINGS_DEFAULT_OLED_ROTATE_180 = false;
constexpr bool SETTINGS_DEFAULT_SWAP_HANDS = false;

struct ServoRanges {
  uint8_t mins[SETTINGS_SERVO_COUNT];
  uint8_t maxs[SETTINGS_SERVO_COUNT];

  // Every angle within 0..SETTINGS_SERVO_ANGLE_MAX and each min below its max.
  bool valid() const;
};

struct ServoCenters {
  float angles[SETTINGS_SERVO_COUNT];

  // Finite angles strictly between the matching safe limits (or 0..180).
  bool valid(const ServoRanges* ranges = nullptr) const;

  // Keep an existing center where possible when the safe limits change.
  void fitTo(const ServoRanges& ranges);
};

// Fields left empty, or nullptr for text, keep their saved value.
struct SettingsPatch {
  std::optional<uint32_t> sleepTimeoutMin;
  const char* hostname = nullptr;
  std::optional<uint8_t> volume;
  std::optional<bool> welcome;
  std::optional<bool> serialLog;
  std::optional<uint32_t> continuousTimeoutMin;
  const char* loading = nullptr;
  const char* eyesStyle = nullptr;
  const char* accessToken = nullptr;
  std::optional<ServoRanges> servoRanges;
  std::optional<ServoCenters> servoCenters;
  std::optional<bool> swapHands;
  const char* rgbOrder = nullptr;
  std::optional<bool> oledRotate180;

  // API name of the first invalid field, or nullptr. Supplied ranges take
  // precedence over savedRanges when validating centers.
  const char* invalidField(const ServoRanges* savedRanges = nullptr) const;

  // Name of the setup wizard step that owns a field the patch sets (RGB order
  // or screen rotation), or nullptr. Those save only in setup AP mode.
  const char* setupOnlyStep() const;
};

void initSettings();

uint32_t settingsSleepTimeoutMin();
uint32_t settingsSleepTimeoutMs();
const char* settingsHostname();
uint8_t settingsVolume();
bool settingsWelcomeEnabled();
bool settingsSerialLogEnabled();
uint32_t settingsContinuousTimeoutMin();
const char* settingsLoading();
const char* settingsEyesStyle();
const char* settingsAccessToken();
bool settingsAccessTokenSet();
const WifiNetworkList& settingsWifiNetworks();
bool settingsWifiConfigured();
float settingsServoMin(int index);
float settingsServoMax(int index);
float settingsServoCenter(int index);
const ServoRanges& settingsServoRanges();
bool settingsSwapHands();
// Output channel from the mapping captured at boot; invalid index returns -1.
int settingsServoChannel(int index);
const char* settingsRgbOrder();
bool settingsOledRotate180();

// True once the saved hostname, loading screen, hand mapping or WiFi networks differ from
// what this boot started with; those apply only after a reboot.
bool settingsRebootRequired();

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutMin);
bool settingsValidateHostname(const char* hostname);
bool settingsValidateVolume(uint8_t volume);
bool settingsValidateContinuousTimeout(uint32_t continuousTimeoutMin);
bool settingsValidateLoading(const char* loading);
bool settingsValidateEyesStyle(const char* eyesStyle);
bool settingsValidateAccessToken(const char* accessToken);
bool settingsValidateWifiSsid(const char* wifiSsid);
bool settingsValidateWifiPassword(const char* wifiPassword);
bool settingsValidateRgbOrder(const char* rgbOrder);

// Saves the fields the patch sets and keeps the others. False when a field is
// invalid (nothing is written) or NVS fails.
bool saveSettings(const SettingsPatch& patch);

// Replaces every saved network in RAM and NVS.
bool saveWifiNetworks(const WifiNetworkList& networks);

// Clears the NVS namespace and restores the defaults, WiFi networks included,
// but keeps servo calibration and hand mapping, RGB LED byte order and OLED 180° rotation.
bool factoryResetSettings();
