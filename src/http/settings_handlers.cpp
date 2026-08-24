#include "http/settings_handlers.h"

#include <Arduino.h>
#include <cstdlib>
#include <cstdio>

#include "http/json.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "settings.h"

namespace {

void sendSettingsJson(
  WebServer& server,
  bool rebootRequired,
  bool wifiConnectSuccess
) {
  char body[704];
  const char* tokenSet =
    settingsAccessTokenSet() ? "true" : "false";
  const char* wifiConfigured =
    settingsWifiConfigured() ? "true" : "false";
  const char* wifiPasswordSet =
    settingsWifiPasswordSet() ? "true" : "false";

  if (wifiConnectSuccess) {
    snprintf(
      body,
      sizeof(body),
      "{"
      "\"ok\":true,"
      "\"sleep_timeout\":%lu,"
      "\"hostname\":\"%s\","
      "\"volume\":%u,"
      "\"welcome\":%s,"
      "\"serial_log\":%s,"
      "\"continuous_timeout\":%lu,"
      "\"loading\":\"%s\","
      "\"access_token_set\":%s,"
      "\"wifi_configured\":%s,"
      "\"wifi_ssid\":\"%s\","
      "\"wifi_password_set\":%s,"
      "\"wifi_connect_success\":true,"
      "\"wifi_ip\":\"%s\","
      "\"wifi_hostname\":\"%s\""
      "%s"
      "}",
      (unsigned long)settingsSleepTimeoutMin(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false",
      settingsSerialLogEnabled() ? "true" : "false",
      (unsigned long)settingsContinuousTimeoutMin(),
      settingsLoading(),
      tokenSet,
      wifiConfigured,
      settingsWifiSsid(),
      wifiPasswordSet,
      wifiIpText(),
      httpMdnsHostname(),
      rebootRequired ? ",\"reboot_required\":true" : ""
    );
  } else if (rebootRequired) {
    snprintf(
      body,
      sizeof(body),
      "{"
      "\"ok\":true,"
      "\"sleep_timeout\":%lu,"
      "\"hostname\":\"%s\","
      "\"volume\":%u,"
      "\"welcome\":%s,"
      "\"serial_log\":%s,"
      "\"continuous_timeout\":%lu,"
      "\"loading\":\"%s\","
      "\"access_token_set\":%s,"
      "\"wifi_configured\":%s,"
      "\"wifi_ssid\":\"%s\","
      "\"wifi_password_set\":%s,"
      "\"reboot_required\":true"
      "}",
      (unsigned long)settingsSleepTimeoutMin(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false",
      settingsSerialLogEnabled() ? "true" : "false",
      (unsigned long)settingsContinuousTimeoutMin(),
      settingsLoading(),
      tokenSet,
      wifiConfigured,
      settingsWifiSsid(),
      wifiPasswordSet
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
      "\"serial_log\":%s,"
      "\"continuous_timeout\":%lu,"
      "\"loading\":\"%s\","
      "\"access_token_set\":%s,"
      "\"wifi_configured\":%s,"
      "\"wifi_ssid\":\"%s\","
      "\"wifi_password_set\":%s"
      "}",
      (unsigned long)settingsSleepTimeoutMin(),
      settingsHostname(),
      static_cast<unsigned>(settingsVolume()),
      settingsWelcomeEnabled() ? "true" : "false",
      settingsSerialLogEnabled() ? "true" : "false",
      (unsigned long)settingsContinuousTimeoutMin(),
      settingsLoading(),
      tokenSet,
      wifiConfigured,
      settingsWifiSsid(),
      wifiPasswordSet
    );
  }

  httpSendJson(server, 200, body);
}

}  // namespace

void handleSettingsGet(WebServer& server) {
  sendSettingsJson(server, false, false);
}

void handleSettingsPost(WebServer& server) {
  const bool hasSleep = server.hasArg("sleep_timeout");
  const bool hasHost = server.hasArg("hostname");
  const bool hasVolume = server.hasArg("volume");
  const bool hasWelcome = server.hasArg("welcome");
  const bool hasSerialLog = server.hasArg("serial_log");
  const bool hasContTo = server.hasArg("continuous_timeout");
  const bool hasLoading = server.hasArg("loading");
  const bool hasAccessToken = server.hasArg("access_token");
  const bool hasWifiSsid = server.hasArg("wifi_ssid");
  const bool hasWifiPassword = server.hasArg("wifi_password");

  if (!hasSleep && !hasHost && !hasVolume && !hasWelcome && !hasSerialLog &&
      !hasContTo && !hasLoading && !hasAccessToken && !hasWifiSsid &&
      !hasWifiPassword) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing sleep_timeout, hostname, volume, welcome, serial_log, continuous_timeout, loading, access_token, wifi_ssid, or wifi_password\"}"
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
  bool serialLog = false;
  const bool* serialLogPtr = nullptr;
  uint32_t continuousTimeoutMin = 0;
  const uint32_t* contToPtr = nullptr;
  String loadingArg;
  const char* loadingPtr = nullptr;
  String accessTokenArg;
  const char* accessTokenPtr = nullptr;
  String wifiSsidArg;
  const char* wifiSsidPtr = nullptr;
  String wifiPasswordArg;
  const char* wifiPasswordPtr = nullptr;
  bool wifiConnectSuccess = false;

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

  if (hasSerialLog) {
    const String serialLogArg = server.arg("serial_log");
    char* end = nullptr;
    const unsigned long parsed =
      strtoul(serialLogArg.c_str(), &end, 10);

    if (end == serialLogArg.c_str() || *end != '\0' || parsed > 1) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid serial_log\"}"
      );
      return;
    }

    serialLog = parsed == 1;
    serialLogPtr = &serialLog;
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

  if (hasWifiSsid || hasWifiPassword) {
    if (!wifiProvisioningMode()) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"wifi setup only in AP mode\"}"
      );
      return;
    }

    if (!hasWifiSsid || !hasWifiPassword) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"wifi_ssid and wifi_password required together\"}"
      );
      return;
    }

    wifiSsidArg = server.arg("wifi_ssid");
    wifiPasswordArg = server.arg("wifi_password");
    wifiSsidPtr = wifiSsidArg.c_str();
    wifiPasswordPtr = wifiPasswordArg.c_str();

    if (!settingsValidateWifiSsid(wifiSsidPtr)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid wifi_ssid\"}"
      );
      return;
    }

    if (!settingsValidateWifiPassword(wifiPasswordPtr)) {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"wifi_password too long\"}"
      );
      return;
    }

    if (!wifiTestCredentials(wifiSsidPtr, wifiPasswordPtr)) {
      char body[96];
      snprintf(
        body,
        sizeof(body),
        "{\"ok\":false,\"error\":\"%s\"}",
        wifiLastConnectError()
      );
      httpSendJson(server, 400, body);
      return;
    }

    wifiConnectSuccess = true;
    wifiSsidPtr = wifiSsidArg.c_str();
    wifiPasswordPtr = wifiPasswordArg.c_str();
  }

  bool rebootRequired = false;

  if (!saveSettings(
        sleepPtr,
        hostPtr,
        volumePtr,
        welcomePtr,
        serialLogPtr,
        contToPtr,
        loadingPtr,
        accessTokenPtr,
        wifiSsidPtr,
        wifiPasswordPtr,
        &rebootRequired
      )) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"save failed\"}"
    );
    return;
  }

  sendSettingsJson(server, rebootRequired, wifiConnectSuccess);
}

void handleSettingsReset(WebServer& server) {
  bool rebootRequired = false;

  if (!factoryResetSettings(&rebootRequired)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"factory reset failed\"}"
    );
    return;
  }

  sendSettingsJson(server, rebootRequired, false);
}

bool isSettingsOrAnimPath(const String& uri) {
  return uri == "/anim" || uri == "/settings" || uri == "/settings/reset" ||
         uri == "/auth";
}
