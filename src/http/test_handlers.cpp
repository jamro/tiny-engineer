#include "http/test_handlers.h"

#include <Arduino.h>

#include "animation.h"
#include "audio.h"
#include "display/oled.h"
#include "http/json.h"
#include "pca9685_servos.h"
#include "rgb.h"
#include "servos.h"
#include "sleep.h"

namespace {

void restoreReadyScreen() {
  showIdleScreen();
}

void handleAudioTest(WebServer& server) {
  touchApiActivity();
  runSoundTest();
  restoreReadyScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"test\":\"audio\"}");
}

void handleBellTest(WebServer& server) {
  touchApiActivity();

  if (!playBell()) {
    restoreReadyScreen();
    httpSendJson(
      server,
      500,
      "{\"ok\":false,\"error\":\"bell playback failed\"}"
    );
    return;
  }

  restoreReadyScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"test\":\"bell\"}");
}

void handleScreenTest(WebServer& server) {
  touchApiActivity();
  runOledTest();
  restoreReadyScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"test\":\"screen\"}");
}

void handleMovementTest(WebServer& server) {
  touchApiActivity();
  runServoTest();
  restoreReadyScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"test\":\"movement\"}");
}

void handleLedTest(WebServer& server) {
  touchApiActivity();
  runRgbTest();
  setRgbForAnimation(getAnimation(), millis());
  restoreReadyScreen();
  httpSendJson(server, 200, "{\"ok\":true,\"test\":\"led\"}");
}

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

void handleServoTest(WebServer& server) {
  touchApiActivity();

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

  if (!moveServoSmooth(index, angle)) {
    httpSendJson(
      server,
      400,
      "{\"ok\":false,\"error\":\"index out of range\"}"
    );
    return;
  }

  restoreReadyScreen();

  char body[96];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"test\":\"servo\",\"index\":%d,\"angle\":%g}",
    index,
    (double)angle
  );

  httpSendJson(server, 200, body);
}

}  // namespace

bool isHttpTestPath(const String& uri) {
  return uri == "/test/audio" ||
    uri == "/test/audio/bell" ||
    uri == "/test/screen" ||
    uri == "/test/movement" ||
    uri == "/test/led" ||
    uri == "/test/servo";
}

void registerHttpTestRoutes(WebServer& server) {
  server.on(
    "/test/audio",
    HTTP_POST,
    [&server]() { handleAudioTest(server); }
  );
  server.on(
    "/test/audio/bell",
    HTTP_POST,
    [&server]() { handleBellTest(server); }
  );
  server.on(
    "/test/screen",
    HTTP_POST,
    [&server]() { handleScreenTest(server); }
  );
  server.on(
    "/test/movement",
    HTTP_POST,
    [&server]() { handleMovementTest(server); }
  );
  server.on(
    "/test/led",
    HTTP_POST,
    [&server]() { handleLedTest(server); }
  );
  server.on(
    "/test/servo",
    HTTP_POST,
    [&server]() { handleServoTest(server); }
  );
}
