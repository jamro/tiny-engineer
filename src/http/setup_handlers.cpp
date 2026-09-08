#include "http/setup_handlers.h"

#include <Arduino.h>
#include <cstdio>

#include "http/json.h"
#include "hardware/pca9685_servos.h"
#include "network/wifi_connect.h"
#include "servos.h"

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

}  // namespace

bool isHttpSetupPath(const String& uri) {
  return uri == "/setup/servo";
}

void registerHttpSetupRoutes(WebServer& server) {
  server.on(
    "/setup/servo",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, handleSetupServo); }
  );
}
