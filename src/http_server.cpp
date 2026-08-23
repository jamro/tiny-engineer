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

void handleAuth() {
  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"required\":%s}",
    settingsAccessTokenSet() ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}

void handleHealth() {
  if (!httpRequireApiAuth(server)) {
    return;
  }

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
  char body[448];
  const char* tokenSet =
    settingsAccessTokenSet() ? "true" : "false";

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
      "\"continuous_timeout\":%lu,"
      "\"loading\":\"%s\","
      "\"access_token_set\":%s,"
      "\"reboot_required\":true"
      "}",
      (unsigned long)settingsSleepTimeoutMin(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false",
      (unsigned long)settingsContinuousTimeoutMin(),
      settingsLoading(),
      tokenSet
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
      "\"welcome\":%s,"
      "\"continuous_timeout\":%lu,"
      "\"loading\":\"%s\","
      "\"access_token_set\":%s"
      "}",
      (unsigned long)settingsSleepTimeoutMin(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false",
      (unsigned long)settingsContinuousTimeoutMin(),
      settingsLoading(),
      tokenSet
    );
  }

  httpSendJson(server, 200, body);
}

void handleSettingsGet() {
  if (!httpRequireApiAuth(server)) {
    return;
  }

  touchApiActivity();
  sendSettingsJson(false);
}

void handleSettingsPost() {
  if (!httpRequireApiAuth(server)) {
    return;
  }

  touchApiActivity();

  const bool hasSleep = server.hasArg("sleep_timeout");
  const bool hasHost = server.hasArg("hostname");
  const bool hasVolume = server.hasArg("volume");
  const bool hasWelcome = server.hasArg("welcome");
  const bool hasContTo = server.hasArg("continuous_timeout");
  const bool hasLoading = server.hasArg("loading");
  const bool hasAccessToken = server.hasArg("access_token");

  if (!hasSleep && !hasHost && !hasVolume && !hasWelcome && !hasContTo &&
      !hasLoading && !hasAccessToken) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing sleep_timeout, hostname, volume, welcome, continuous_timeout, loading, or access_token\"}"
    );
    return;
  }

  uint32_t sleepTimeoutMin = 0;
  const uint32_t* sleepPtr = nullptr;
  String hostnameArg;
  const char* hostPtr = nullptr;
  uint8_t volume = 0;
  const uint8_t* volumePtr = nullptr;
  bool welcome = false;
  const bool* welcomePtr = nullptr;
  uint32_t continuousTimeoutMin = 0;
  const uint32_t* contToPtr = nullptr;
  String loadingArg;
  const char* loadingPtr = nullptr;
  String accessTokenArg;
  const char* accessTokenPtr = nullptr;

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

    sleepTimeoutMin = static_cast<uint32_t>(parsed);

    if (!settingsValidateSleepTimeout(sleepTimeoutMin)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"sleep_timeout out of range\"}"
      );
      return;
    }

    sleepPtr = &sleepTimeoutMin;
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

  if (hasContTo) {
    const String contArg = server.arg("continuous_timeout");
    char* end = nullptr;
    const unsigned long parsed =
      strtoul(contArg.c_str(), &end, 10);

    if (end == contArg.c_str() || *end != '\0') {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid continuous_timeout\"}"
      );
      return;
    }

    continuousTimeoutMin = static_cast<uint32_t>(parsed);

    if (!settingsValidateContinuousTimeout(continuousTimeoutMin)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"continuous_timeout out of range\"}"
      );
      return;
    }

    contToPtr = &continuousTimeoutMin;
  }

  if (hasLoading) {
    loadingArg = server.arg("loading");
    loadingPtr = loadingArg.c_str();

    if (!settingsValidateLoading(loadingPtr)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid loading\"}"
      );
      return;
    }
  }

  if (hasAccessToken) {
    accessTokenArg = server.arg("access_token");
    accessTokenPtr = accessTokenArg.c_str();

    if (!settingsValidateAccessToken(accessTokenPtr)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid access_token\"}"
      );
      return;
    }
  }

  bool rebootRequired = false;

  if (!saveSettings(
        sleepPtr,
        hostPtr,
        volumePtr,
        welcomePtr,
        contToPtr,
        loadingPtr,
        accessTokenPtr,
        &rebootRequired
      )) {
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
  if (!httpRequireApiAuth(server)) {
    return;
  }

  touchApiActivity();
  sendAnimationJson();
}

void handleAnimPost() {
  if (!httpRequireApiAuth(server)) {
    return;
  }

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
  return uri == "/anim" || uri == "/settings" || uri == "/auth";
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    httpSendCorsPreflight(server);
    return;
  }

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

  static const char* kCollectHeaders[] = {"Authorization"};
  server.collectHeaders(kCollectHeaders, 1);

  server.on("/", HTTP_GET, handleIndex);
  server.on("/animations", HTTP_GET, handleIndex);
  server.on("/servo", HTTP_GET, handleIndex);
  server.on("/tests", HTTP_GET, handleIndex);
  server.on("/api", HTTP_GET, handleIndex);
  server.on("/config", HTTP_GET, handleIndex);
  server.on("/auth", HTTP_GET, handleAuth);
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
