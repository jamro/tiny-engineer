#include "api/ops.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "animation.h"
#include "audio/audio.h"
#include "display/oled.h"
#include "hardware/pca9685_servos.h"
#include "hardware/rgb.h"
#include "servos.h"

namespace {

void restoreReadyScreen() {
  showIdleScreen();
}

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

void apiOpTestAudio(const ApiRequest& /*req*/, ApiResponse& resp) {
  runSoundTest();
  restoreReadyScreen();
  resp.set(200, "{\"ok\":true,\"test\":\"audio\"}");
}

void apiOpTestBell(const ApiRequest& /*req*/, ApiResponse& resp) {
  if (!playBell()) {
    restoreReadyScreen();
    resp.setError(500, "bell playback failed");
    return;
  }
  restoreReadyScreen();
  resp.set(200, "{\"ok\":true,\"test\":\"bell\"}");
}

void apiOpTestScreen(const ApiRequest& /*req*/, ApiResponse& resp) {
  runOledTest();
  restoreReadyScreen();
  resp.set(200, "{\"ok\":true,\"test\":\"screen\"}");
}

void apiOpTestMovement(const ApiRequest& /*req*/, ApiResponse& resp) {
  runServoTest();
  restoreReadyScreen();
  resp.set(200, "{\"ok\":true,\"test\":\"movement\"}");
}

void apiOpTestLed(const ApiRequest& /*req*/, ApiResponse& resp) {
  runRgbTest();
  setRgbForAnimation(getAnimation(), millis());
  restoreReadyScreen();
  resp.set(200, "{\"ok\":true,\"test\":\"led\"}");
}

void apiOpTestServo(const ApiRequest& req, ApiResponse& resp) {
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

  if (!moveServoSmooth(index, angle)) {
    resp.setError(400, "index out of range");
    return;
  }

  restoreReadyScreen();

  char body[96];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"test\":\"servo\",\"index\":%d,\"angle\":%g}",
    index,
    (double)angle
  );
  resp.set(200, body);
}
