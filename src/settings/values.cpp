#include "settings/values.h"

#include "serial_log.h"
#include "servos.h"

SettingsValues g_settings = SettingsValues::defaults();
WifiNetworkList g_wifiNetworks;
SettingsValues g_bootSettings = g_settings;
bool g_wifiChangedSinceBoot = false;

SettingsValues SettingsValues::defaults() {
  SettingsValues values{};

  values.sleepTimeoutMin = SETTINGS_DEFAULT_SLEEP_TIMEOUT_MIN;
  strlcpy(values.hostname, SETTINGS_DEFAULT_HOSTNAME, sizeof(values.hostname));
  values.volume = SETTINGS_DEFAULT_VOLUME;
  values.welcome = SETTINGS_DEFAULT_WELCOME;
  values.serialLog = SETTINGS_DEFAULT_SERIAL_LOG;
  values.continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  strlcpy(values.loading, SETTINGS_DEFAULT_LOADING, sizeof(values.loading));
  strlcpy(values.eyesStyle, SETTINGS_DEFAULT_EYES_STYLE, sizeof(values.eyesStyle));
  strlcpy(values.accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN, sizeof(values.accessToken));

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    values.servoRanges.mins[i] = static_cast<uint8_t>(SERVO_SPECS[i].min);
    values.servoRanges.maxs[i] = static_cast<uint8_t>(SERVO_SPECS[i].max);
    values.servoCenters.angles[i] = servoMid(SERVO_SPECS[i]);
  }

  strlcpy(values.rgbOrder, SETTINGS_DEFAULT_RGB_ORDER, sizeof(values.rgbOrder));
  values.oledRotate180 = SETTINGS_DEFAULT_OLED_ROTATE_180;
  values.swapHands = SETTINGS_DEFAULT_SWAP_HANDS;
  return values;
}

void SettingsValues::apply(const SettingsPatch& patch) {
  sleepTimeoutMin = patch.sleepTimeoutMin.value_or(sleepTimeoutMin);
  volume = patch.volume.value_or(volume);
  welcome = patch.welcome.value_or(welcome);
  serialLog = patch.serialLog.value_or(serialLog);
  continuousTimeoutMin = patch.continuousTimeoutMin.value_or(continuousTimeoutMin);
  servoRanges = patch.servoRanges.value_or(servoRanges);
  servoCenters = patch.servoCenters.value_or(servoCenters);
  swapHands = patch.swapHands.value_or(swapHands);

  if (patch.servoRanges && !patch.servoCenters) {
    servoCenters.fitTo(servoRanges);
  }
  oledRotate180 = patch.oledRotate180.value_or(oledRotate180);

  if (patch.hostname != nullptr) {
    strlcpy(hostname, patch.hostname, sizeof(hostname));
  }

  if (patch.loading != nullptr) {
    strlcpy(loading, patch.loading, sizeof(loading));
  }

  if (patch.eyesStyle != nullptr) {
    strlcpy(eyesStyle, patch.eyesStyle, sizeof(eyesStyle));
  }

  if (patch.accessToken != nullptr) {
    strlcpy(accessToken, patch.accessToken, sizeof(accessToken));
  }

  if (patch.rgbOrder != nullptr) {
    strlcpy(rgbOrder, patch.rgbOrder, sizeof(rgbOrder));
  }
}

void SettingsValues::log(const char* prefix) const {
  if (!serialLog) {
    return;
  }

  const ServoRanges& r = servoRanges;

  Serial.printf(
    "%s sleep_timeout=%umin hostname=%s volume=%u welcome=%s serial_log=on "
    "continuous_timeout=%umin loading=%s eyes_style=%s access_token=%s wifi_networks=%u "
    "servos=%u-%u;%u-%u;%u-%u;%u-%u;%u-%u "
    "servo_centers=%.2f;%.2f;%.2f;%.2f;%.2f swap_hands=%s rgb_order=%s oled_rotate_180=%s\n",
    prefix,
    static_cast<unsigned>(sleepTimeoutMin),
    hostname,
    volume,
    welcome ? "on" : "off",
    static_cast<unsigned>(continuousTimeoutMin),
    loading,
    eyesStyle,
    accessToken[0] != '\0' ? "set" : "unset",
    static_cast<unsigned>(g_wifiNetworks.count()),
    r.mins[0], r.maxs[0], r.mins[1], r.maxs[1], r.mins[2], r.maxs[2], r.mins[3], r.maxs[3],
    r.mins[4], r.maxs[4],
    servoCenters.angles[0], servoCenters.angles[1], servoCenters.angles[2],
    servoCenters.angles[3], servoCenters.angles[4],
    swapHands ? "on" : "off",
    rgbOrder,
    oledRotate180 ? "on" : "off"
  );
}
