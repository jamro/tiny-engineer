#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "audio.h"
#include "http_server.h"
#include "oled.h"
#include "pca9685_servos.h"
#include "rgb.h"
#include "wifi_connect.h"

namespace {

WebServer server(80);

constexpr const char* HOSTNAME = "tiny-engineer.local";

void restoreReadyScreen() {
  showOledText(
    "ROBOT READY",
    wifiConnected() ? wifiIpText() : "WIFI FAIL"
  );
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
  runSoundTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"audio\"}");
}

void handleScreenTest() {
  runOledTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"screen\"}");
}

void handleMovementTest() {
  runServoTest();
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"movement\"}");
}

void handleLedTest() {
  runRgbTest();
  setRgb(0, 32, 0);
  restoreReadyScreen();
  sendJson(200, "{\"ok\":true,\"test\":\"led\"}");
}

bool isTestPath(const String& uri) {
  return uri == "/test/audio" ||
    uri == "/test/screen" ||
    uri == "/test/movement" ||
    uri == "/test/led";
}

void handleNotFound() {
  if (isTestPath(server.uri())) {
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
  server.on("/test/audio", HTTP_POST, handleAudioTest);
  server.on("/test/screen", HTTP_POST, handleScreenTest);
  server.on("/test/movement", HTTP_POST, handleMovementTest);
  server.on("/test/led", HTTP_POST, handleLedTest);
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
