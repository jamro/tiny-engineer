#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "animation.h"
#include "http/json.h"
#include "http/test_handlers.h"
#include "http_server.h"
#include "display/oled.h"
#include "sleep.h"
#include "wifi_connect.h"

namespace {

WebServer server(80);

constexpr const char* HOSTNAME = "tiny-engineer.local";

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

void handleAnimGet() {
  touchApiActivity();
  sendAnimationJson();
}

void handleAnimPost() {
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

bool isAnimPath(const String& uri) {
  return uri == "/anim";
}

void handleNotFound() {
  touchApiActivity();

  if (isHttpTestPath(server.uri()) || isAnimPath(server.uri())) {
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

  server.on("/", HTTP_GET, handleHealth);
  server.on("/anim", HTTP_GET, handleAnimGet);
  server.on("/anim", HTTP_POST, handleAnimPost);
  registerHttpTestRoutes(server);
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
