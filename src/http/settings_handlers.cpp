#include <Arduino.h>

#include <algorithm>

#include "api/commands.h"
#include "display/oled.h"
#include "hardware/rgb.h"
#include "http/handlers.h"
#include "http/request.h"
#include "http/response.h"
#include "http/server.h"
#include "network/wifi_connect.h"
#include "serial_log.h"
#include "settings/settings.h"
#include "settings/wifi_network_list.h"

#include <cstring>

namespace {

template <typename T>
bool readField(JsonVariantConst value, std::optional<T>& out) {
  if (!value.is<T>()) {
    return false;
  }

  out = value.as<T>();
  return true;
}

bool readField(JsonVariantConst value, const char*& out) {
  out = value.as<const char*>();
  return out != nullptr;
}

bool readField(JsonVariantConst list, uint8_t (&out)[SETTINGS_SERVO_COUNT]) {
  if (!list.is<JsonArrayConst>() || list.size() != SETTINGS_SERVO_COUNT) {
    return false;
  }

  for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
    if (!list[i].is<uint8_t>()) {
      return false;
    }

    out[i] = list[i].as<uint8_t>();
  }

  return true;
}

// Fills the patch from the body. Returns the 400 message, or an empty string
// when every field is known and has the right JSON type.
String readPatch(JsonObjectConst body, SettingsPatch& patch) {
  const char* known[16] = {};
  size_t knownCount = 0;
  const char* wrong = nullptr;

  auto read = [&](const char* key, auto& out) {
    // A missing key keeps the saved value; an explicit null is a wrong type.
    if (body[key].isUnbound()) {
      return;
    }

    known[knownCount++] = key;

    if (wrong == nullptr && !readField(body[key], out)) {
      wrong = key;
    }
  };

  read("sleep_timeout", patch.sleepTimeoutMin);
  read("hostname", patch.hostname);
  read("volume", patch.volume);
  read("welcome", patch.welcome);
  read("serial_log", patch.serialLog);
  read("continuous_timeout", patch.continuousTimeoutMin);
  read("loading", patch.loading);
  read("eyes_style", patch.eyesStyle);
  read("access_token", patch.accessToken);
  read("link_mode", patch.linkMode);
  read("rgb_order", patch.rgbOrder);
  read("oled_rotate_180", patch.oledRotate180);

  ServoRanges ranges{};
  read("servo_mins", ranges.mins);
  read("servo_maxs", ranges.maxs);

  for (const JsonPairConst field : body) {
    const auto isKnown = [&](const char* key) { return field.key() == key; };

    if (std::none_of(known, known + knownCount, isKnown)) {
      return String("unknown field ") + field.key().c_str();
    }
  }

  if (wrong != nullptr) {
    return String("invalid ") + wrong;
  }

  const bool hasMins = !body["servo_mins"].isUnbound();

  if (hasMins != !body["servo_maxs"].isUnbound()) {
    return "servo_mins and servo_maxs go together";
  }

  if (hasMins) {
    patch.servoRanges = ranges;
  }

  return "";
}

JsonDocument settingsJson() {
  JsonDocument doc;

  doc["ok"] = true;
  doc["sleep_timeout"] = settingsSleepTimeoutMin();
  doc["hostname"] = settingsHostname();
  doc["volume"] = settingsVolume();
  doc["welcome"] = settingsWelcomeEnabled();
  doc["serial_log"] = settingsSerialLogEnabled();
  doc["continuous_timeout"] = settingsContinuousTimeoutMin();
  doc["loading"] = settingsLoading();
  doc["eyes_style"] = settingsEyesStyle();
  doc["access_token_set"] = settingsAccessTokenSet();
  doc["link_mode"] = settingsLinkMode();
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

  doc["reboot_required"] = settingsRebootRequired();
  return doc;
}

}  // namespace

void handleSettingsGet(WebServer& server) {
  sendJson(server, 200, settingsJson());
}

void handleSettingsPost(WebServer& server) {
  JsonDocument body;

  if (!readJsonBody(server, body)) {
    return;
  }

  if (body.size() == 0) {
    sendError(server, 400, "send at least one setting");
    return;
  }

  SettingsPatch patch;
  String error = readPatch(body.as<JsonObjectConst>(), patch);

  if (error.isEmpty() && patch.invalidField() != nullptr) {
    error = String("invalid ") + patch.invalidField();
  }

  if (!error.isEmpty()) {
    sendError(server, 400, error.c_str());
    return;
  }

  const char* setupStep = patch.setupOnlyStep();

  if (setupStep != nullptr && !wifiProvisioningMode()) {
    sendError(server, 400, (String(setupStep) + " setup only in AP mode").c_str());
    return;
  }

  if (const char* linkErr = settingsLinkModePatchDenied(
        patch.linkMode,
        settingsLinkMode(),
        settingsProvisioned(),
        wifiProvisioningMode()
      );
      linkErr != nullptr) {
    sendError(server, 400, linkErr);
    return;
  }

  if (!saveSettings(patch)) {
    sendError(server, 400, "save failed");
    return;
  }

  if (patch.oledRotate180) {
    applyOledRotation();
    restoreProvisioningOled();
  }

  const bool leaveUsbOnly =
    wifiProvisioningMode() && patch.linkMode != nullptr && strcmp(patch.linkMode, "usb") == 0;

  sendJson(server, 200, settingsJson());

  // Mirror Wi‑Fi setup teardown: reply first, then drop the config AP so the
  // browser gets the 200. Clears provisioning so serial unlocks without reboot.
  if (leaveUsbOnly) {
    delay(300);
    wifiStopProvisioningAp();
    stopHttpServer();
    showTextThenIdle("USB-only", "CLI serial");
    setRgb(0, 0, 0);
    serialLogPrintln("USB-only mode: setup AP stopped; use serial CLI");
  }
}

void handleSettingsWifiPut(WebServer& server) {
  JsonDocument body;

  if (!readJsonBody<JsonArrayConst>(server, body)) {
    return;
  }

  const JsonArrayConst entries = body.as<JsonArrayConst>();

  // An empty list would put the robot in setup AP mode at the next boot;
  // factory reset is the way to clear WiFi.
  if (entries.size() == 0) {
    sendError(server, 400, "send at least one wifi network");
    return;
  }

  const WifiNetworkList& saved = settingsWifiNetworks();
  WifiNetworkList next;

  for (const JsonObjectConst entry : entries) {
    const JsonVariantConst passwordField = entry["password"];
    // A missing key keeps the saved password; an explicit null is an error.
    const bool keepsPassword = passwordField.isUnbound();

    if (!entry["ssid"].is<const char*>() ||
        !(keepsPassword || passwordField.is<const char*>())) {
      sendError(server, 400, "each network needs a string ssid and an optional string password");
      return;
    }

    const char* ssid = entry["ssid"];
    const char* password = keepsPassword ? saved.passwordFor(ssid)
                                         : passwordField.as<const char*>();

    if (password == nullptr) {
      sendError(server, 400, "password required for a new network");
      return;
    }

    if (!next.add(ssid, password)) {
      sendError(server, 400, "invalid ssid or password, duplicate ssid, or more than 5 networks");
      return;
    }
  }

  // Outside setup AP mode a test would drop the connection serving this
  // request, so the list is saved untested.
  const bool tested = wifiProvisioningMode();

  if (tested &&
      !wifiTestCredentials(next.at(0).ssid, next.at(0).password)) {
    sendError(server, 400, wifiLastConnectError());
    return;
  }

  if (!saveWifiNetworks(next)) {
    if (tested) {
      wifiRestoreProvisioningAp();
    }

    sendError(server, 400, "save failed");
    return;
  }

  if (tested) {
    SettingsPatch mode;
    mode.linkMode = "wifi";
    if (!saveSettings(mode)) {
      sendError(server, 400, "save failed");
      return;
    }
  }

  JsonDocument reply = settingsJson();

  if (tested) {
    reply["wifi_connect_success"] = true;
    reply["wifi_ip"] = wifiIpText();
    reply["wifi_hostname"] = wifiMdnsHostname();
  }

  sendJson(server, 200, reply);

  if (tested) {
    delay(300);
    wifiStopProvisioningAp();
    showBootIpThenIdle(wifiIpText());
  }
}

void handleSettingsReset(WebServer& server) {
  const ApiResult result = apiFactoryReset();
  if (!result.ok) {
    sendError(server, result.code, result.error);
    return;
  }

  sendJson(server, 200, settingsJson());
}
