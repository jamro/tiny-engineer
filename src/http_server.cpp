#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cstdio>
#include <cstdlib>

#include "animation.h"
#include "http/index_page.h"
#include "http/json.h"
#include "http/test_handlers.h"
#include "http_server.h"
#include "display/oled.h"
#include "settings.h"
#include "sleep.h"
#include "wifi_connect.h"

namespace {

WebServer server(80);

char mdnsHostname[SETTINGS_HOSTNAME_MAX_LEN + 7];

void refreshMdnsHostname() {
  snprintf(
    mdnsHostname,
    sizeof(mdnsHostname),
    "%s.local",
    settingsHostname()
  );
}

void handleIndex() {
  sendIndexPage(server);
}

void handleHealth() {
  touchApiActivity();
  const bool wifiOk =
    WiFi.status() == WL_CONNECTED;

  refreshMdnsHostname();

  char body[360];

  snprintf(
    body,
    sizeof(body),
    "{"
    "\"ok\":true,"
    "\"uptime_ms\":%lu,"
    "\"free_heap\":%u,"
    "\"heap_size\":%u,"
    "\"wifi\":{"
    "\"connected\":%s,"
    "\"ip\":\"%s\","
    "\"rssi\":%d,"
    "\"hostname\":\"%s\""
    "},"
    "\"oled\":%s"
    "}",
    (unsigned long)millis(),
    (unsigned)ESP.getFreeHeap(),
    (unsigned)ESP.getHeapSize(),
    wifiOk ? "true" : "false",
    wifiOk ? wifiIpText() : "",
    wifiOk ? (int)WiFi.RSSI() : 0,
    mdnsHostname,
    oledAvailable ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}

void sendAnimationJson() {
  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"animation\":\"%s\"}",
    animationName(getAnimation())
  );

  httpSendJson(server, 200, body);
}

void sendSettingsJson(bool rebootRequired) {
  char body[256];

  if (rebootRequired) {
    snprintf(
      body,
      sizeof(body),
      "{"
      "\"ok\":true,"
      "\"sleep_timeout\":%lu,"
      "\"hostname\":\"%s\","
      "\"volume\":%u,"
      "\"welcome\":%s,"
      "\"reboot_required\":true"
      "}",
      (unsigned long)settingsSleepTimeoutS(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false"
    );
  } else {
    snprintf(
      body,
      sizeof(body),
      "{"
      "\"ok\":true,"
      "\"sleep_timeout\":%lu,"
      "\"hostname\":\"%s\","
      "\"volume\":%u,"
      "\"welcome\":%s"
      "}",
      (unsigned long)settingsSleepTimeoutS(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false"
    );
  }

  httpSendJson(server, 200, body);
}

void handleSettingsGet() {
  touchApiActivity();
  sendSettingsJson(false);
}

void handleSettingsPost() {
  touchApiActivity();

  const bool hasSleep = server.hasArg("sleep_timeout");
  const bool hasHost = server.hasArg("hostname");
  const bool hasVolume = server.hasArg("volume");
  const bool hasWelcome = server.hasArg("welcome");

  if (!hasSleep && !hasHost && !hasVolume && !hasWelcome) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing sleep_timeout, hostname, volume, or welcome\"}"
    );
    return;
  }

  uint32_t sleepTimeoutS = 0;
  const uint32_t* sleepPtr = nullptr;
  String hostnameArg;
  const char* hostPtr = nullptr;
  uint8_t volume = 0;
  const uint8_t* volumePtr = nullptr;
  bool welcome = false;
  const bool* welcomePtr = nullptr;

  if (hasSleep) {
    const String sleepArg = server.arg("sleep_timeout");
    char* end = nullptr;
    const unsigned long parsed =
      strtoul(sleepArg.c_str(), &end, 10);

    if (end == sleepArg.c_str() || *end != '\0') {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid sleep_timeout\"}"
      );
      return;
    }

    sleepTimeoutS = static_cast<uint32_t>(parsed);

    if (!settingsValidateSleepTimeout(sleepTimeoutS)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"sleep_timeout out of range\"}"
      );
      return;
    }

    sleepPtr = &sleepTimeoutS;
  }

  if (hasHost) {
    hostnameArg = server.arg("hostname");
    hostPtr = hostnameArg.c_str();

    if (!settingsValidateHostname(hostPtr)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid hostname\"}"
      );
      return;
    }
  }

  if (hasVolume) {
    const String volumeArg = server.arg("volume");
    char* end = nullptr;
    const unsigned long parsed =
      strtoul(volumeArg.c_str(), &end, 10);

    if (end == volumeArg.c_str() || *end != '\0' || parsed > 255) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid volume\"}"
      );
      return;
    }

    volume = static_cast<uint8_t>(parsed);

    if (!settingsValidateVolume(volume)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"volume out of range\"}"
      );
      return;
    }

    volumePtr = &volume;
  }

  if (hasWelcome) {
    const String welcomeArg = server.arg("welcome");
    char* end = nullptr;
    const unsigned long parsed =
      strtoul(welcomeArg.c_str(), &end, 10);

    if (end == welcomeArg.c_str() || *end != '\0' || parsed > 1) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid welcome\"}"
      );
      return;
    }

    welcome = parsed == 1;
    welcomePtr = &welcome;
  }

  bool rebootRequired = false;

  if (!saveSettings(sleepPtr, hostPtr, volumePtr, welcomePtr, &rebootRequired)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"save failed\"}"
    );
    return;
  }

  sendSettingsJson(rebootRequired);
}

void handleAnimGet() {
  touchApiActivity();
  sendAnimationJson();
}

void handleAnimPost() {
  touchApiActivity();

  if (!server.hasArg("name")) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing name\"}"
    );
    return;
  }

  AnimationId id;

  if (!parseAnimationName(server.arg("name").c_str(), id)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"unknown animation\"}"
    );
    return;
  }

  setAnimation(id);
  sendAnimationJson();
}

bool isSettingsOrAnimPath(const String& uri) {
  return uri == "/anim" || uri == "/settings";
}

void handleNotFound() {
  touchApiActivity();

  if (isHttpTestPath(server.uri()) || isSettingsOrAnimPath(server.uri())) {
    httpSendJson(
      server,
      405,
      "{\"ok\":false,\"error\":\"method not allowed\"}"
    );
    return;
  }

  httpSendJson(
    server,
    404,
    "{\"ok\":false,\"error\":\"not found\"}"
  );
}

}  // namespace

void startHttpServer() {
  if (!wifiConnected()) {
    Serial.println("HTTP: skipped (no wifi)");
    return;
  }

  refreshMdnsHostname();

  server.on("/", HTTP_GET, handleIndex);
  server.on("/animations", HTTP_GET, handleIndex);
  server.on("/servo", HTTP_GET, handleIndex);
  server.on("/tests", HTTP_GET, handleIndex);
  server.on("/api", HTTP_GET, handleIndex);
  server.on("/config", HTTP_GET, handleIndex);
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/anim", HTTP_GET, handleAnimGet);
  server.on("/anim", HTTP_POST, handleAnimPost);
  server.on("/settings", HTTP_GET, handleSettingsGet);
  server.on("/settings", HTTP_POST, handleSettingsPost);
  registerHttpTestRoutes(server);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.print("HTTP: http://");
  Serial.print(wifiIpText());
  Serial.println("/");
  Serial.print("HTTP: http://");
  Serial.print(mdnsHostname);
  Serial.println("/");
  Serial.print("HTTP: http://");
  Serial.print(wifiIpText());
  Serial.println("/health");
  Serial.print("HTTP: http://");
  Serial.print(mdnsHostname);
  Serial.println("/health");
}

void pollHttpServer() {
  if (!wifiConnected()) {
    delay(10);
    return;
  }

  server.handleClient();
}
