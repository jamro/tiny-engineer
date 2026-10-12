#include "http/settings_handlers.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

#include "api/http_bridge.h"
#include "api/ops.h"
#include "api/request.h"
#include "display/oled.h"
#include "http/json.h"
#include "network/wifi_connect.h"
#include "settings/settings.h"
#include "settings/wifi_network_list.h"

void handleSettingsGet(WebServer& server) {
  httpDispatchApi(server);
}

void handleSettingsPost(WebServer& server) {
  httpDispatchApi(server);
}

void handleSettingsWifiPut(WebServer& server) {
  JsonDocument body;

  if (deserializeJson(body, server.arg("plain")) ||
      !body.is<JsonArrayConst>()) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"body must be a JSON array of networks\"}"
    );
    return;
  }

  const JsonArrayConst entries = body.as<JsonArrayConst>();

  // An empty list would put the robot in setup AP mode at the next boot;
  // factory reset is the way to clear WiFi.
  if (entries.size() == 0) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"send at least one wifi network\"}"
    );
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
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"each network needs a string ssid and an optional string password\"}"
      );
      return;
    }

    const char* ssid = entry["ssid"];
    const char* password = keepsPassword ? saved.passwordFor(ssid)
                                         : passwordField.as<const char*>();

    if (password == nullptr) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"password required for a new network\"}"
      );
      return;
    }

    if (!next.add(ssid, password)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid ssid or password, duplicate ssid, or more than 5 networks\"}"
      );
      return;
    }
  }

  // Outside setup AP mode a test would drop the connection serving this
  // request, so the list is saved untested.
  const bool tested = wifiProvisioningMode();

  if (tested &&
      !wifiTestCredentials(next.at(0).ssid, next.at(0).password)) {
    char error[96];
    snprintf(
      error,
      sizeof(error),
      "{\"ok\":false,\"error\":\"%s\"}",
      wifiLastConnectError()
    );
    httpSendJson(server, 400, error);
    return;
  }

  if (!saveWifiNetworks(next)) {
    if (tested) {
      wifiRestoreProvisioningAp();
    }

    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"save failed\"}"
    );
    return;
  }

  // Static: ApiResponse is ~3KB; keep it off the ~8KB loopTask stack.
  static ApiResponse resp;
  apiFillSettingsResponse(resp, false, tested);
  httpSendJson(server, resp.status, resp.body);

  if (tested) {
    delay(300);
    wifiStopProvisioningAp();
    showBootIp(wifiIpText());
  }
}

void handleSettingsReset(WebServer& server) {
  httpDispatchApi(server);
}

bool isSettingsOrAnimPath(const String& uri) {
  return uri == "/anim" || uri == "/play" || uri.startsWith("/play/") ||
         uri == "/settings" || uri == "/settings/reset" ||
         uri == "/settings/wifi" ||
         uri == "/auth" || uri == "/setup/servo" ||
         uri == "/servos/park";
}
