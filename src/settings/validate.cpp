#include "settings/settings.h"

#include <cmath>
#include <cstring>

#include "display/eyes/styles/eye_style.h"

bool settingsValidateSleepTimeout(uint32_t sleepTimeoutMin) {
  return sleepTimeoutMin >= SETTINGS_SLEEP_TIMEOUT_MIN_MIN &&
         sleepTimeoutMin <= SETTINGS_SLEEP_TIMEOUT_MAX_MIN;
}

bool settingsValidateHostname(const char* hostname) {
  if (hostname == nullptr) {
    return false;
  }

  const size_t len = strlen(hostname);

  if (len < 1 || len > SETTINGS_HOSTNAME_MAX_LEN) {
    return false;
  }

  if (hostname[0] == '-' || hostname[len - 1] == '-') {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    const char c = hostname[i];
    const bool ok =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-';

    if (!ok) {
      return false;
    }
  }

  return true;
}

bool settingsValidateVolume(uint8_t volume) {
  return volume <= SETTINGS_VOLUME_MAX;
}

bool settingsValidateContinuousTimeout(uint32_t continuousTimeoutMin) {
  return continuousTimeoutMin >= SETTINGS_CONTINUOUS_TIMEOUT_MIN_MIN &&
         continuousTimeoutMin <= SETTINGS_CONTINUOUS_TIMEOUT_MAX_MIN;
}

bool settingsValidateLoading(const char* loading) {
  if (loading == nullptr) {
    return false;
  }

  return strcmp(loading, "progress") == 0 ||
         strcmp(loading, "sleep_inertia") == 0;
}

bool settingsValidateEyesStyle(const char* eyesStyle) {
  return isRegisteredEyeStyleId(eyesStyle);
}

bool settingsValidateAccessToken(const char* accessToken) {
  if (accessToken == nullptr) {
    return false;
  }

  const size_t len = strlen(accessToken);

  if (len > SETTINGS_ACCESS_TOKEN_MAX_LEN) {
    return false;
  }

  // HTTP clients and the server trim header values, so a token that starts or
  // ends with a space could never match the Authorization header.
  if (len > 0 && (accessToken[0] == ' ' || accessToken[len - 1] == ' ')) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    const unsigned char c =
      static_cast<unsigned char>(accessToken[i]);

    if (c < 0x20 || c > 0x7E) {
      return false;
    }
  }

  return true;
}

bool settingsValidateWifiSsid(const char* wifiSsid) {
  if (wifiSsid == nullptr) {
    return false;
  }

  const size_t len = strlen(wifiSsid);

  return len >= 1 && len <= SETTINGS_WIFI_SSID_MAX_LEN;
}

bool settingsValidateWifiPassword(const char* wifiPassword) {
  if (wifiPassword == nullptr) {
    return false;
  }

  return strlen(wifiPassword) <= SETTINGS_WIFI_PASSWORD_MAX_LEN;
}

bool ServoRanges::valid() const {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (maxs[i] > SETTINGS_SERVO_ANGLE_MAX || mins[i] >= maxs[i]) {
      return false;
    }
  }

  return true;
}

bool ServoCenters::valid(const ServoRanges* ranges) const {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    const float min = ranges != nullptr ? ranges->mins[i] : 0.0f;
    const float max = ranges != nullptr ? ranges->maxs[i] : SETTINGS_SERVO_ANGLE_MAX;

    if (!std::isfinite(angles[i]) || angles[i] <= min || angles[i] >= max) {
      return false;
    }
  }

  return true;
}

void ServoCenters::fitTo(const ServoRanges& ranges) {
  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (!std::isfinite(angles[i]) || angles[i] <= ranges.mins[i] || angles[i] >= ranges.maxs[i]) {
      angles[i] = (ranges.mins[i] + ranges.maxs[i]) * 0.5f;
    }
  }
}

bool settingsValidateRgbOrder(const char* rgbOrder) {
  if (rgbOrder == nullptr) {
    return false;
  }

  return strcmp(rgbOrder, "RGB") == 0 ||
         strcmp(rgbOrder, "RBG") == 0 ||
         strcmp(rgbOrder, "GRB") == 0 ||
         strcmp(rgbOrder, "GBR") == 0 ||
         strcmp(rgbOrder, "BRG") == 0 ||
         strcmp(rgbOrder, "BGR") == 0;
}

const char* SettingsPatch::invalidField(const ServoRanges* savedRanges) const {
  if (sleepTimeoutMin && !settingsValidateSleepTimeout(*sleepTimeoutMin)) {
    return "sleep_timeout";
  }

  if (hostname != nullptr && !settingsValidateHostname(hostname)) {
    return "hostname";
  }

  if (volume && !settingsValidateVolume(*volume)) {
    return "volume";
  }

  if (continuousTimeoutMin && !settingsValidateContinuousTimeout(*continuousTimeoutMin)) {
    return "continuous_timeout";
  }

  if (loading != nullptr && !settingsValidateLoading(loading)) {
    return "loading";
  }

  if (eyesStyle != nullptr && !settingsValidateEyesStyle(eyesStyle)) {
    return "eyes_style";
  }

  if (accessToken != nullptr && !settingsValidateAccessToken(accessToken)) {
    return "access_token";
  }

  if (servoRanges && !servoRanges->valid()) {
    return "servo_mins or servo_maxs";
  }

  if (servoCenters && !servoCenters->valid(servoRanges ? &*servoRanges : savedRanges)) {
    return "servo_centers";
  }

  if (rgbOrder != nullptr && !settingsValidateRgbOrder(rgbOrder)) {
    return "rgb_order";
  }

  return nullptr;
}

const char* SettingsPatch::setupOnlyStep() const {
  if (rgbOrder != nullptr) {
    return "rgb";
  }

  if (oledRotate180) {
    return "oled";
  }

  return nullptr;
}
