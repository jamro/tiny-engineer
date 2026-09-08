#include "http/setup_handlers.h"

#include <Arduino.h>
#include <cstdio>

#include "http/json.h"
#include "hardware/pca9685_servos.h"
#include "hardware/rgb.h"
#include "audio/audio.h"
#include "display/oled.h"
#include "network/wifi_connect.h"
#include "servos.h"
#include "settings/settings.h"

namespace {

bool isDigitsOnly(const String& s) {
  if (s.length() == 0) {
    return false;
  }

  for (unsigned i = 0; i < s.length(); i++) {
    if (s[i] < '0' || s[i] > '9') {
      return false;
    }
  }

  return true;
}

bool isValidFloatArg(const String& s) {
  if (s.length() == 0) {
    return false;
  }

  unsigned i = 0;

  if (s[0] == '+' || s[0] == '-') {
    i = 1;
    if (i >= s.length()) {
      return false;
    }
  }

  bool sawDigit = false;
  bool sawDot = false;

  for (; i < s.length(); i++) {
    const char c = s[i];

    if (c >= '0' && c <= '9') {
      sawDigit = true;
      continue;
    }

    if (c == '.' && !sawDot) {
      sawDot = true;
      continue;
    }

    return false;
  }

  return sawDigit;
}

void handleSetupServo(WebServer& server) {
  if (!wifiProvisioningMode()) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"setup servo only in AP mode\"}"
    );
    return;
  }

  if (server.hasArg("all")) {
    if (server.arg("all") != "90") {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid all\"}"
      );
      return;
    }

    servoMoveAllToElectricalAngle(90.0f);
    httpSendJson(server, 200, "{\"ok\":true,\"setup\":\"servo\",\"all\":90}");
    return;
  }

  if (!server.hasArg("index") || !server.hasArg("angle")) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"missing index or angle\"}"
    );
    return;
  }

  const String indexArg = server.arg("index");
  const String angleArg = server.arg("angle");

  if (!isDigitsOnly(indexArg)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"invalid index\"}"
    );
    return;
  }

  if (!isValidFloatArg(angleArg)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"invalid angle\"}"
    );
    return;
  }

  const int index = indexArg.toInt();
  const float angle = angleArg.toFloat();

  if (index < 0 || index >= SERVO_COUNT) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"index out of range\"}"
    );
    return;
  }

  if (angle < 0.0f || angle > 180.0f) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"angle out of range\"}"
    );
    return;
  }

  if (!moveServoSmoothElectrical(index, angle)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"index out of range\"}"
    );
    return;
  }

  char body[96];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"setup\":\"servo\",\"index\":%d,\"angle\":%g}",
    index,
    (double)angle
  );

  httpSendJson(server, 200, body);
}

void handleSetupLed(WebServer& server) {
  if (!wifiProvisioningMode()) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"setup led only in AP mode\"}"
    );
    return;
  }

  const bool hasColor = server.hasArg("color");
  const bool hasByte = server.hasArg("byte");
  const String byteArg = hasByte ? server.arg("byte") : "";
  const bool byteOff = hasByte && byteArg == "off";

  if (hasColor && hasByte && !byteOff) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"color and byte conflict\"}"
    );
    return;
  }

  if (hasColor) {
    const String colorArg = server.arg("color");
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (colorArg == "R") {
      r = 255;
    } else if (colorArg == "G") {
      g = 255;
    } else if (colorArg == "B") {
      b = 255;
    } else {
      httpSendJson(
        server,
        400,
        "{\"ok\":false,\"error\":\"invalid color\"}"
      );
      return;
    }

    const char* orderPtr = nullptr;
    String orderArg;

    if (server.hasArg("rgb_order")) {
      orderArg = server.arg("rgb_order");

      if (!settingsValidateRgbOrder(orderArg.c_str())) {
        httpSendJson(
          server,
          400,
          "{\"ok\":false,\"error\":\"invalid rgb_order\"}"
        );
        return;
      }

      orderPtr = orderArg.c_str();
    }

    rgbSetupHoldLogical(r, g, b, orderPtr);

    char body[72];

    snprintf(
      body,
      sizeof(body),
      "{\"ok\":true,\"setup\":\"led\",\"color\":\"%s\"}",
      colorArg.c_str()
    );

    httpSendJson(server, 200, body);
    return;
  }

  if (!hasByte || byteOff) {
    rgbSetupRelease();
    httpSendJson(server, 200, "{\"ok\":true,\"setup\":\"led\",\"off\":true}");
    return;
  }

  if (!isDigitsOnly(byteArg)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"invalid byte\"}"
    );
    return;
  }

  const int byteIndex = byteArg.toInt();

  if (byteIndex < 0 || byteIndex > 2) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"byte out of range\"}"
    );
    return;
  }

  rgbSetupHoldWireByte(static_cast<uint8_t>(byteIndex));

  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"setup\":\"led\",\"byte\":%d}",
    byteIndex
  );

  httpSendJson(server, 200, body);
}

void handleSetupAudio(WebServer& server) {
  if (!wifiProvisioningMode()) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"setup audio only in AP mode\"}"
    );
    return;
  }

  if (!playWelcome()) {
    showIdleScreen();
    httpSendJson(
      server,
      500,
      "{\"ok\":false,\"error\":\"welcome playback failed\"}"
    );
    return;
  }

  showIdleScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"setup\":\"audio\"}");
}

}  // namespace

bool isHttpSetupPath(const String& uri) {
  return uri == "/setup/servo" || uri == "/setup/led" || uri == "/setup/audio";
}

void registerHttpSetupRoutes(WebServer& server) {
  server.on(
    "/setup/servo",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, handleSetupServo); }
  );
  server.on(
    "/setup/led",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, handleSetupLed); }
  );
  server.on(
    "/setup/audio",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, handleSetupAudio); }
  );
}
