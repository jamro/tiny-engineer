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
  values.pushButton = SETTINGS_DEFAULT_PUSH_BUTTON;
  values.pushButtonPin = SETTINGS_DEFAULT_PUSH_BUTTON_PIN;
  strlcpy(
    values.pushButtonSoundDown,
    SETTINGS_DEFAULT_PUSH_BUTTON_SOUND,
    sizeof(values.pushButtonSoundDown)
  );
  strlcpy(
    values.pushButtonSoundUp,
    SETTINGS_DEFAULT_PUSH_BUTTON_SOUND,
    sizeof(values.pushButtonSoundUp)
  );
  strlcpy(
    values.pushButtonSoundClick,
    SETTINGS_DEFAULT_PUSH_BUTTON_SOUND,
    sizeof(values.pushButtonSoundClick)
  );
  strlcpy(
    values.pushButtonSoundDoubleClick,
    SETTINGS_DEFAULT_PUSH_BUTTON_SOUND,
    sizeof(values.pushButtonSoundDoubleClick)
  );
  strlcpy(
    values.pushButtonSoundLongClick,
    SETTINGS_DEFAULT_PUSH_BUTTON_SOUND,
    sizeof(values.pushButtonSoundLongClick)
  );
  values.continuousTimeoutMin = SETTINGS_DEFAULT_CONTINUOUS_TIMEOUT_MIN;
  strlcpy(values.loading, SETTINGS_DEFAULT_LOADING, sizeof(values.loading));
  strlcpy(values.eyesStyle, SETTINGS_DEFAULT_EYES_STYLE, sizeof(values.eyesStyle));
  strlcpy(values.accessToken, SETTINGS_DEFAULT_ACCESS_TOKEN, sizeof(values.accessToken));
  strlcpy(values.linkMode, SETTINGS_DEFAULT_LINK_MODE, sizeof(values.linkMode));

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    values.servoRanges.mins[i] = static_cast<uint8_t>(SERVO_SPECS[i].min);
    values.servoRanges.maxs[i] = static_cast<uint8_t>(SERVO_SPECS[i].max);
  }

  strlcpy(values.rgbOrder, SETTINGS_DEFAULT_RGB_ORDER, sizeof(values.rgbOrder));
  values.oledRotate180 = SETTINGS_DEFAULT_OLED_ROTATE_180;
  return values;
}

void SettingsValues::apply(const SettingsPatch& patch) {
  sleepTimeoutMin = patch.sleepTimeoutMin.value_or(sleepTimeoutMin);
  volume = patch.volume.value_or(volume);
  welcome = patch.welcome.value_or(welcome);
  serialLog = patch.serialLog.value_or(serialLog);
  pushButton = patch.pushButton.value_or(pushButton);
  pushButtonPin = patch.pushButtonPin.value_or(pushButtonPin);
  continuousTimeoutMin = patch.continuousTimeoutMin.value_or(continuousTimeoutMin);

  if (patch.pushButtonSoundDown != nullptr) {
    strlcpy(pushButtonSoundDown, patch.pushButtonSoundDown, sizeof(pushButtonSoundDown));
  }

  if (patch.pushButtonSoundUp != nullptr) {
    strlcpy(pushButtonSoundUp, patch.pushButtonSoundUp, sizeof(pushButtonSoundUp));
  }

  if (patch.pushButtonSoundClick != nullptr) {
    strlcpy(pushButtonSoundClick, patch.pushButtonSoundClick, sizeof(pushButtonSoundClick));
  }

  if (patch.pushButtonSoundDoubleClick != nullptr) {
    strlcpy(
      pushButtonSoundDoubleClick,
      patch.pushButtonSoundDoubleClick,
      sizeof(pushButtonSoundDoubleClick)
    );
  }

  if (patch.pushButtonSoundLongClick != nullptr) {
    strlcpy(
      pushButtonSoundLongClick,
      patch.pushButtonSoundLongClick,
      sizeof(pushButtonSoundLongClick)
    );
  }
  servoRanges = patch.servoRanges.value_or(servoRanges);
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

  if (patch.linkMode != nullptr) {
    strlcpy(linkMode, patch.linkMode, sizeof(linkMode));
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
  char line[512];
  snprintf(
    line,
    sizeof(line),
    "%s sleep_timeout=%umin hostname=%s volume=%u welcome=%s serial_log=on "
    "push_button=%s push_button_pin=%u "
    "btn_snd=%s/%s/%s/%s/%s "
    "continuous_timeout=%umin loading=%s eyes_style=%s "
    "access_token=%s link_mode=%s wifi_networks=%u servos=%u-%u;%u-%u;%u-%u;%u-%u;%u-%u "
    "rgb_order=%s oled_rotate_180=%s",
    prefix,
    static_cast<unsigned>(sleepTimeoutMin),
    hostname,
    volume,
    welcome ? "on" : "off",
    pushButton ? "on" : "off",
    static_cast<unsigned>(pushButtonPin),
    pushButtonSoundDown,
    pushButtonSoundUp,
    pushButtonSoundClick,
    pushButtonSoundDoubleClick,
    pushButtonSoundLongClick,
    static_cast<unsigned>(continuousTimeoutMin),
    loading,
    eyesStyle,
    accessToken[0] != '\0' ? "set" : "unset",
    linkMode[0] != '\0' ? linkMode : "(none)",
    static_cast<unsigned>(g_wifiNetworks.count()),
    r.mins[0], r.maxs[0], r.mins[1], r.maxs[1], r.mins[2], r.maxs[2], r.mins[3], r.maxs[3],
    r.mins[4], r.maxs[4],
    rgbOrder,
    oledRotate180 ? "on" : "off"
  );
  serialLogPrintln(line);
}
