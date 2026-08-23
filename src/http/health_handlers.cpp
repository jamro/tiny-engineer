#include "http/health_handlers.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstdio>

#include "display/oled.h"
#include "http/json.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "settings.h"

void handleAuth() {
  WebServer& server = httpServer();
  char body[64];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"required\":%s}",
    settingsAccessTokenSet() ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}

void handleHealth(WebServer& server) {
  const bool wifiOk =
    WiFi.status() == WL_CONNECTED;

  refreshMdnsHostname();

  char body[360];

  snprintf(
    body,
    sizeof(body),
    "{"
    "\"ok\":true,"
    "\"uptime_ms\":%lu,"
    "\"free_heap\":%u,"
    "\"heap_size\":%u,"
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
    (unsigned)ESP.getHeapSize(),
    wifiOk ? "true" : "false",
    wifiOk ? wifiIpText() : "",
    wifiOk ? (int)WiFi.RSSI() : 0,
    httpMdnsHostname(),
    oledAvailable ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}
