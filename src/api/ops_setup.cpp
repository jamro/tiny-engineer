#include "api/ops.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "audio/audio.h"
#include "display/oled.h"
#include "hardware/pca9685_servos.h"
#include "hardware/rgb.h"
#include "network/wifi_connect.h"
#include "servos.h"
#include "settings/settings.h"

namespace {

bool isDigitsOnly(const char* s) {
  if (s == nullptr || *s == '\0') {
    return false;
  }
  for (const char* p = s; *p != '\0'; p++) {
    if (*p < '0' || *p > '9') {
      return false;
    }
  }
  return true;
}

bool isValidFloatArg(const char* s) {
  if (s == nullptr || *s == '\0') {
    return false;
  }

  size_t i = 0;
  if (s[0] == '+' || s[0] == '-') {
    i = 1;
    if (s[i] == '\0') {
      return false;
    }
  }

  bool sawDigit = false;
  bool sawDot = false;

  for (; s[i] != '\0'; i++) {
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

}  // namespace

void apiOpSetupServo(const ApiRequest& req, ApiResponse& resp) {
  if (!wifiProvisioningMode()) {
    resp.setError(400, "setup servo only in AP mode");
    return;
  }

  if (req.hasArg("all")) {
    if (std::strcmp(req.arg("all"), "90") != 0) {
      resp.setError(400, "invalid all");
      return;
    }
    servoMoveAllToElectricalAngle(90.0f);
    resp.set(200, "{\"ok\":true,\"setup\":\"servo\",\"all\":90}");
    return;
  }

  if (!req.hasArg("index") || !req.hasArg("angle")) {
    resp.setError(400, "missing index or angle");
    return;
  }

  const char* indexArg = req.arg("index");
  const char* angleArg = req.arg("angle");

  if (!isDigitsOnly(indexArg)) {
    resp.setError(400, "invalid index");
    return;
  }

  if (!isValidFloatArg(angleArg)) {
    resp.setError(400, "invalid angle");
    return;
  }

  const int index = static_cast<int>(std::strtol(indexArg, nullptr, 10));
  const float angle = static_cast<float>(std::strtod(angleArg, nullptr));

  if (index < 0 || index >= SERVO_COUNT) {
    resp.setError(400, "index out of range");
    return;
  }

  if (angle < 0.0f || angle > 180.0f) {
    resp.setError(400, "angle out of range");
    return;
  }

  if (!moveServoSmoothElectrical(index, angle)) {
    resp.setError(400, "index out of range");
    return;
  }

  char body[96];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"setup\":\"servo\",\"index\":%d,\"angle\":%g}",
    index,
    (double)angle
  );
  resp.set(200, body);
}

void apiOpSetupLed(const ApiRequest& req, ApiResponse& resp) {
  if (!wifiProvisioningMode()) {
    resp.setError(400, "setup led only in AP mode");
    return;
  }

  const bool hasColor = req.hasArg("color");
  const bool hasByte = req.hasArg("byte");
  const char* byteArg = hasByte ? req.arg("byte") : "";
  const bool byteOff = hasByte && std::strcmp(byteArg, "off") == 0;

  if (hasColor && hasByte && !byteOff) {
    resp.setError(400, "color and byte conflict");
    return;
  }

  if (hasColor) {
    const char* colorArg = req.arg("color");
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (std::strcmp(colorArg, "R") == 0) {
      r = 255;
    } else if (std::strcmp(colorArg, "G") == 0) {
      g = 255;
    } else if (std::strcmp(colorArg, "B") == 0) {
      b = 255;
    } else {
      resp.setError(400, "invalid color");
      return;
    }

    const char* orderPtr = nullptr;
    if (req.hasArg("rgb_order")) {
      orderPtr = req.arg("rgb_order");
      if (!settingsValidateRgbOrder(orderPtr)) {
        resp.setError(400, "invalid rgb_order");
        return;
      }
    }

    rgbSetupHoldLogical(r, g, b, orderPtr);

    char body[72];
    std::snprintf(
      body,
      sizeof(body),
      "{\"ok\":true,\"setup\":\"led\",\"color\":\"%s\"}",
      colorArg
    );
    resp.set(200, body);
    return;
  }

  if (!hasByte || byteOff) {
    rgbSetupRelease();
    resp.set(200, "{\"ok\":true,\"setup\":\"led\",\"off\":true}");
    return;
  }

  if (!isDigitsOnly(byteArg)) {
    resp.setError(400, "invalid byte");
    return;
  }

  const int byteIndex = static_cast<int>(std::strtol(byteArg, nullptr, 10));
  if (byteIndex < 0 || byteIndex > 2) {
    resp.setError(400, "byte out of range");
    return;
  }

  rgbSetupHoldWireByte(static_cast<uint8_t>(byteIndex));

  char body[64];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"setup\":\"led\",\"byte\":%d}",
    byteIndex
  );
  resp.set(200, body);
}

void apiOpSetupAudio(const ApiRequest& /*req*/, ApiResponse& resp) {
  if (!wifiProvisioningMode()) {
    resp.setError(400, "setup audio only in AP mode");
    return;
  }

  if (!playWelcome()) {
    showIdleScreen();
    resp.setError(500, "welcome playback failed");
    return;
  }

  showIdleScreen();
  resp.set(200, "{\"ok\":true,\"setup\":\"audio\"}");
}

void apiOpSetupOled(const ApiRequest& req, ApiResponse& resp) {
  if (!wifiProvisioningMode()) {
    resp.setError(400, "setup oled only in AP mode");
    return;
  }

  if (!req.hasArg("rotate_180")) {
    restoreProvisioningOled();
    resp.set(200, "{\"ok\":true,\"setup\":\"oled\",\"restored\":true}");
    return;
  }

  const char* rotArg = req.arg("rotate_180");
  if (!isDigitsOnly(rotArg)) {
    resp.setError(400, "invalid rotate_180");
    return;
  }

  const int parsed = static_cast<int>(std::strtol(rotArg, nullptr, 10));
  if (parsed != 0 && parsed != 1) {
    resp.setError(400, "invalid rotate_180");
    return;
  }

  const bool rotate180 = parsed == 1;
  showOledOrientationTest(rotate180);

  char body[72];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"setup\":\"oled\",\"rotate_180\":%s}",
    rotate180 ? "true" : "false"
  );
  resp.set(200, body);
}
