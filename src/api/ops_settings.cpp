#include "api/ops.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "display/oled.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "settings/settings.h"
#include "settings/wifi_network_list.h"

namespace {

bool parseServoCsv(const char* raw, uint8_t out[SETTINGS_SERVO_COUNT]) {
  const char* p = raw;

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (p == nullptr || *p == '\0') {
      return false;
    }

    char* end = nullptr;
    const unsigned long parsed = strtoul(p, &end, 10);

    if (end == p || parsed > 255) {
      return false;
    }

    if (i + 1 < SETTINGS_SERVO_COUNT) {
      if (*end != ',') {
        return false;
      }
      p = end + 1;
    } else if (*end != '\0') {
      return false;
    }

    out[i] = static_cast<uint8_t>(parsed);
  }

  return true;
}

}  // namespace

void apiFillSettingsResponse(
  ApiResponse& resp,
  bool rebootRequired,
  bool wifiConnectSuccess
) {
  JsonDocument doc;

  doc["ok"] = true;
  doc["sleep_timeout"] = settingsSleepTimeoutMin();
  doc["hostname"] = settingsHostname();
  doc["volume"] = settingsVolume();
  doc["welcome"] = settingsWelcomeEnabled();
  doc["serial_log"] = settingsSerialLogEnabled();
  doc["servo_hold"] = settingsServoHold();
  doc["wifi_enabled"] = settingsWifiEnabled();
  doc["continuous_timeout"] = settingsContinuousTimeoutMin();
  doc["loading"] = settingsLoading();
  doc["eyes_style"] = settingsEyesStyle();
  doc["access_token_set"] = settingsAccessTokenSet();
  doc["wifi_configured"] = settingsWifiConfigured();

  JsonArray servoMins = doc["servo_mins"].to<JsonArray>();
  JsonArray servoMaxs = doc["servo_maxs"].to<JsonArray>();

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    servoMins.add(static_cast<unsigned>(settingsServoMin(i)));
    servoMaxs.add(static_cast<unsigned>(settingsServoMax(i)));
  }

  doc["rgb_order"] = settingsRgbOrder();
  doc["oled_rotate_180"] = settingsOledRotate180();

  const WifiNetworkList& networks = settingsWifiNetworks();
  JsonArray ssids = doc["wifi_networks"].to<JsonArray>();

  for (size_t i = 0; i < networks.count(); i++) {
    ssids.add(networks.at(i).ssid);
  }

  if (wifiConnectSuccess) {
    doc["wifi_connect_success"] = true;
    doc["wifi_ip"] = wifiIpText();
    doc["wifi_hostname"] = httpMdnsHostname();
  }

  if (rebootRequired) {
    doc["reboot_required"] = true;
  }

  const size_t needed = measureJson(doc);
  if (needed >= ApiResponse::kBodyMax) {
    resp.setError(500, "settings payload too large");
    return;
  }

  serializeJson(doc, resp.body, ApiResponse::kBodyMax);
  resp.status = 200;
}

void apiOpSettingsGet(const ApiRequest& /*req*/, ApiResponse& resp) {
  apiFillSettingsResponse(resp, false, false);
}

void apiOpSettingsPost(const ApiRequest& req, ApiResponse& resp) {
  const bool hasSleep = req.hasArg("sleep_timeout");
  const bool hasHost = req.hasArg("hostname");
  const bool hasVolume = req.hasArg("volume");
  const bool hasWelcome = req.hasArg("welcome");
  const bool hasSerialLog = req.hasArg("serial_log");
  const bool hasServoHold = req.hasArg("servo_hold");
  const bool hasWifiEnabled = req.hasArg("wifi_enabled");
  const bool hasContTo = req.hasArg("continuous_timeout");
  const bool hasLoading = req.hasArg("loading");
  const bool hasEyesStyle = req.hasArg("eyes_style");
  const bool hasAccessToken = req.hasArg("access_token");
  const bool hasServoMins = req.hasArg("servo_mins");
  const bool hasServoMaxs = req.hasArg("servo_maxs");
  const bool hasRgbOrder = req.hasArg("rgb_order");
  const bool hasOledRotate180 = req.hasArg("oled_rotate_180");

  if (!hasSleep && !hasHost && !hasVolume && !hasWelcome && !hasSerialLog &&
      !hasServoHold && !hasWifiEnabled &&
      !hasContTo && !hasLoading && !hasEyesStyle && !hasAccessToken &&
      !hasServoMins && !hasServoMaxs &&
      !hasRgbOrder && !hasOledRotate180) {
    resp.setError(
      400,
      "missing sleep_timeout, hostname, volume, welcome, serial_log, servo_hold, wifi_enabled, continuous_timeout, loading, eyes_style, access_token, servo_mins, servo_maxs, rgb_order, or oled_rotate_180"
    );
    return;
  }

  uint32_t sleepTimeoutMin = 0;
  const uint32_t* sleepPtr = nullptr;
  const char* hostPtr = nullptr;
  uint8_t volume = 0;
  const uint8_t* volumePtr = nullptr;
  bool welcome = false;
  const bool* welcomePtr = nullptr;
  bool serialLog = false;
  const bool* serialLogPtr = nullptr;
  bool servoHold = false;
  const bool* servoHoldPtr = nullptr;
  bool wifiEnabled = false;
  const bool* wifiEnabledPtr = nullptr;
  uint32_t continuousTimeoutMin = 0;
  const uint32_t* contToPtr = nullptr;
  const char* loadingPtr = nullptr;
  const char* eyesStylePtr = nullptr;
  const char* accessTokenPtr = nullptr;
  uint8_t servoMins[SETTINGS_SERVO_COUNT] = {};
  uint8_t servoMaxs[SETTINGS_SERVO_COUNT] = {};
  const uint8_t* servoMinsPtr = nullptr;
  const uint8_t* servoMaxsPtr = nullptr;
  const char* rgbOrderPtr = nullptr;
  bool oledRotate180 = false;
  const bool* oledRotate180Ptr = nullptr;

  if (hasSleep) {
    const char* sleepArg = req.arg("sleep_timeout");
    char* end = nullptr;
    const unsigned long parsed = strtoul(sleepArg, &end, 10);

    if (end == sleepArg || *end != '\0') {
      resp.setError(400, "invalid sleep_timeout");
      return;
    }

    sleepTimeoutMin = static_cast<uint32_t>(parsed);
    if (!settingsValidateSleepTimeout(sleepTimeoutMin)) {
      resp.setError(400, "sleep_timeout out of range");
      return;
    }
    sleepPtr = &sleepTimeoutMin;
  }

  if (hasHost) {
    hostPtr = req.arg("hostname");
    if (!settingsValidateHostname(hostPtr)) {
      resp.setError(400, "invalid hostname");
      return;
    }
  }

  if (hasVolume) {
    const char* volumeArg = req.arg("volume");
    char* end = nullptr;
    const unsigned long parsed = strtoul(volumeArg, &end, 10);

    if (end == volumeArg || *end != '\0' || parsed > 255) {
      resp.setError(400, "invalid volume");
      return;
    }

    volume = static_cast<uint8_t>(parsed);
    if (!settingsValidateVolume(volume)) {
      resp.setError(400, "volume out of range");
      return;
    }
    volumePtr = &volume;
  }

  if (hasWelcome) {
    const char* welcomeArg = req.arg("welcome");
    char* end = nullptr;
    const unsigned long parsed = strtoul(welcomeArg, &end, 10);

    if (end == welcomeArg || *end != '\0' || parsed > 1) {
      resp.setError(400, "invalid welcome");
      return;
    }
    welcome = parsed == 1;
    welcomePtr = &welcome;
  }

  if (hasSerialLog) {
    const char* serialLogArg = req.arg("serial_log");
    char* end = nullptr;
    const unsigned long parsed = strtoul(serialLogArg, &end, 10);

    if (end == serialLogArg || *end != '\0' || parsed > 1) {
      resp.setError(400, "invalid serial_log");
      return;
    }
    serialLog = parsed == 1;
    serialLogPtr = &serialLog;
  }

  if (hasServoHold) {
    const char* servoHoldArg = req.arg("servo_hold");
    char* end = nullptr;
    const unsigned long parsed = strtoul(servoHoldArg, &end, 10);

    if (end == servoHoldArg || *end != '\0' || parsed > 1) {
      resp.setError(400, "invalid servo_hold");
      return;
    }
    servoHold = parsed == 1;
    servoHoldPtr = &servoHold;
  }

  if (hasWifiEnabled) {
    const char* wifiEnabledArg = req.arg("wifi_enabled");
    char* end = nullptr;
    const unsigned long parsed = strtoul(wifiEnabledArg, &end, 10);

    if (end == wifiEnabledArg || *end != '\0' || parsed > 1) {
      resp.setError(400, "invalid wifi_enabled");
      return;
    }
    wifiEnabled = parsed == 1;
    wifiEnabledPtr = &wifiEnabled;
  }

  if (hasContTo) {
    const char* contArg = req.arg("continuous_timeout");
    char* end = nullptr;
    const unsigned long parsed = strtoul(contArg, &end, 10);

    if (end == contArg || *end != '\0') {
      resp.setError(400, "invalid continuous_timeout");
      return;
    }

    continuousTimeoutMin = static_cast<uint32_t>(parsed);
    if (!settingsValidateContinuousTimeout(continuousTimeoutMin)) {
      resp.setError(400, "continuous_timeout out of range");
      return;
    }
    contToPtr = &continuousTimeoutMin;
  }

  if (hasLoading) {
    loadingPtr = req.arg("loading");
    if (!settingsValidateLoading(loadingPtr)) {
      resp.setError(400, "invalid loading");
      return;
    }
  }

  if (hasEyesStyle) {
    eyesStylePtr = req.arg("eyes_style");
    if (!settingsValidateEyesStyle(eyesStylePtr)) {
      resp.setError(400, "invalid eyes_style");
      return;
    }
  }

  if (hasAccessToken) {
    accessTokenPtr = req.arg("access_token");
    if (!settingsValidateAccessToken(accessTokenPtr)) {
      resp.setError(400, "invalid access_token");
      return;
    }
  }

  if (hasServoMins || hasServoMaxs) {
    if (!wifiProvisioningMode() && settingsBootWifiEnabled()) {
      resp.setError(400, "servo setup only in AP mode");
      return;
    }

    if (!hasServoMins || !hasServoMaxs) {
      resp.setError(400, "servo_mins and servo_maxs required together");
      return;
    }

    if (!parseServoCsv(req.arg("servo_mins"), servoMins) ||
        !parseServoCsv(req.arg("servo_maxs"), servoMaxs)) {
      resp.setError(400, "invalid servo_mins or servo_maxs");
      return;
    }

    if (!settingsValidateServoRanges(servoMins, servoMaxs)) {
      resp.setError(400, "servo ranges out of range");
      return;
    }

    servoMinsPtr = servoMins;
    servoMaxsPtr = servoMaxs;
  }

  if (hasRgbOrder) {
    if (!wifiProvisioningMode() && settingsBootWifiEnabled()) {
      resp.setError(400, "rgb setup only in AP mode");
      return;
    }

    rgbOrderPtr = req.arg("rgb_order");
    if (!settingsValidateRgbOrder(rgbOrderPtr)) {
      resp.setError(400, "invalid rgb_order");
      return;
    }
  }

  if (hasOledRotate180) {
    if (!wifiProvisioningMode() && settingsBootWifiEnabled()) {
      resp.setError(400, "oled setup only in AP mode");
      return;
    }

    const char* oledArg = req.arg("oled_rotate_180");
    char* end = nullptr;
    const unsigned long parsed = strtoul(oledArg, &end, 10);

    if (end == oledArg || *end != '\0' || parsed > 1) {
      resp.setError(400, "invalid oled_rotate_180");
      return;
    }

    oledRotate180 = parsed == 1;
    oledRotate180Ptr = &oledRotate180;
  }

  bool rebootRequired = false;

  if (!saveSettings(
        sleepPtr,
        hostPtr,
        volumePtr,
        welcomePtr,
        serialLogPtr,
        servoHoldPtr,
        wifiEnabledPtr,
        contToPtr,
        loadingPtr,
        eyesStylePtr,
        accessTokenPtr,
        servoMinsPtr,
        servoMaxsPtr,
        rgbOrderPtr,
        oledRotate180Ptr,
        &rebootRequired
      )) {
    resp.setError(400, "save failed");
    return;
  }

  refreshMdnsHostname();

  if (oledRotate180Ptr != nullptr) {
    applyOledRotation();
    restoreProvisioningOled();
  }

  apiFillSettingsResponse(resp, rebootRequired, false);
}

void apiOpSettingsReset(const ApiRequest& /*req*/, ApiResponse& resp) {
  bool rebootRequired = false;

  if (!factoryResetSettings(&rebootRequired)) {
    resp.setError(400, "factory reset failed");
    return;
  }

  apiFillSettingsResponse(resp, rebootRequired, false);
}
