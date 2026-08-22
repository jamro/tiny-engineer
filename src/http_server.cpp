#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "animation.h"
#include "audio.h"
#include "http_server.h"
#include "oled.h"
#include "pca9685_servos.h"
#include "rgb.h"
#include "servos.h"
#include "sleep.h"
#include "wifi_connect.h"

namespace {

WebServer server(80);

constexpr const char* HOSTNAME = "tiny-engineer.local";

void restoreReadyScreen() {
  showIdleScreen();
}

void sendJson(int code, const char* body) {
  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );
  server.send(
    code,
    "application/json",
    body
  );
}

void handleHealth() {
  touchApiActivity();
  const bool wifiOk =
    WiFi.status() == WL_CONNECTED;

  char body[320];

  snprintf(
    body,
    sizeof(body),
    "{"
    "\"ok\":true,"
    "\"uptime_ms\":%lu,"
    "\"free_heap\":%u,"
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
    wifiOk ? "true" : "false",
    wifiOk ? wifiIpText() : "",
    wifiOk ? (int)WiFi.RSSI() : 0,
    HOSTNAME,
    oledAvailable ? "true" : "false"
  );

  sendJson(200, body);
}

void handleAudioTest() {
  touchApiActivity();
  runSoundTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"audio\"}");
}

void handleBellTest() {
  touchApiActivity();

  if (!playBell()) {
    restoreReadyScreen();
    sendJson(
      500,
      "{\"ok\":false,\"error\":\"bell playback failed\"}"
    );
    return;
  }

  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"bell\"}");
}

void handleScreenTest() {
  touchApiActivity();
  runOledTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"screen\"}");
}

void handleMovementTest() {
  touchApiActivity();
  runServoTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"movement\"}");
}

void handleLedTest() {
  touchApiActivity();
  runRgbTest();
  setRgbForAnimation(getAnimation(), millis());
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"led\"}");
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

void handleServoTest() {
  touchApiActivity();

  if (!server.hasArg("index") || !server.hasArg("angle")) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"missing index or angle\"}"
    );
    return;
  }

  const String indexArg = server.arg("index");
  const String angleArg = server.arg("angle");

  if (!isDigitsOnly(indexArg)) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"invalid index\"}"
    );
    return;
  }

  if (!isValidFloatArg(angleArg)) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"invalid angle\"}"
    );
    return;
  }

  const int index = indexArg.toInt();
  const float angle = angleArg.toFloat();

  if (index < 0 || index >= SERVO_COUNT) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"index out of range\"}"
    );
    return;
  }

  if (angle < 0.0f || angle > 180.0f) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"angle out of range\"}"
    );
    return;
  }

  if (!moveServoSmooth(index, angle)) {
    sendJson(
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

  sendJson(200, body);
}

void sendAnimationJson() {
  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"animation\":\"%s\"}",
    animationName(getAnimation())
  );

  sendJson(200, body);
}

void handleAnimGet() {
  touchApiActivity();
  sendAnimationJson();
}

void handleAnimPost() {
  touchApiActivity();

  if (!server.hasArg("name")) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"missing name\"}"
    );
    return;
  }

  AnimationId id;

  if (!parseAnimationName(server.arg("name").c_str(), id)) {
    sendJson(
      400,
      "{\"ok\":false,\"error\":\"unknown animation\"}"
    );
    return;
  }

  setAnimation(id);
  sendAnimationJson();
}

bool isTestPath(const String& uri) {
  return uri == "/test/audio" ||
    uri == "/test/audio/bell" ||
    uri == "/test/screen" ||
    uri == "/test/movement" ||
    uri == "/test/led" ||
    uri == "/test/servo";
}

bool isAnimPath(const String& uri) {
  return uri == "/anim";
}

void handleNotFound() {
  touchApiActivity();

  if (isTestPath(server.uri()) || isAnimPath(server.uri())) {
    sendJson(
      405,
      "{\"ok\":false,\"error\":\"method not allowed\"}"
    );
    return;
  }

  sendJson(
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

  server.on("/", HTTP_GET, handleHealth);
  server.on("/anim", HTTP_GET, handleAnimGet);
  server.on("/anim", HTTP_POST, handleAnimPost);
  server.on("/test/audio", HTTP_POST, handleAudioTest);
  server.on("/test/audio/bell", HTTP_POST, handleBellTest);
  server.on("/test/screen", HTTP_POST, handleScreenTest);
  server.on("/test/movement", HTTP_POST, handleMovementTest);
  server.on("/test/led", HTTP_POST, handleLedTest);
  server.on("/test/servo", HTTP_POST, handleServoTest);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.print("HTTP: http://");
  Serial.print(wifiIpText());
  Serial.println("/");
  Serial.print("HTTP: http://");
  Serial.print(HOSTNAME);
  Serial.println("/");
}

void pollHttpServer() {
  if (!wifiConnected()) {
    delay(10);
    return;
  }

  server.handleClient();
}
