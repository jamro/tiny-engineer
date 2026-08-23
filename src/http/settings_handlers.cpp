#include "http/settings_handlers.h"

#include <Arduino.h>
#include <cstdlib>
#include <cstdio>

#include "http/json.h"
#include "settings.h"

namespace {

void sendSettingsJson(WebServer& server, bool rebootRequired) {
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

}  // namespace

void handleSettingsGet(WebServer& server) {
  sendSettingsJson(server, false);
}

void handleSettingsPost(WebServer& server) {
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

  sendSettingsJson(server, rebootRequired);
}

bool isSettingsOrAnimPath(const String& uri) {
  return uri == "/anim" || uri == "/settings" || uri == "/auth";
}
