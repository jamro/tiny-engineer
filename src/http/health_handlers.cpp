#include "http/health_handlers.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstdio>

#include "display/oled.h"
#include "hardware/chip_temp.h"
#include "http/json.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "settings.h"

void handleAuth() {
  WebServer& server = httpServer();
  char body[128];

  snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"required\":%s,\"wifi_configured\":%s,\"provisioning\":%s}",
    settingsAccessTokenSet() ? "true" : "false",
    settingsWifiConfigured() ? "true" : "false",
    wifiProvisioningMode() ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}

void handleHealth(WebServer& server) {
  const bool wifiOk =
    WiFi.status() == WL_CONNECTED;

  refreshMdnsHostname();

  float tempC = 0.0f;
  const bool tempOk = chipTempCelsius(&tempC);
  char tempFrag[24];
  if (tempOk) {
    snprintf(tempFrag, sizeof(tempFrag), "%.1f", tempC);
  } else {
    snprintf(tempFrag, sizeof(tempFrag), "null");
  }

  char body[512];

  snprintf(
    body,
    sizeof(body),
    "{"
    "\"ok\":true,"
    "\"uptime_ms\":%lu,"
    "\"free_heap\":%u,"
    "\"heap_size\":%u,"
    "\"cpu_temp_c\":%s,"
    "\"wifi\":{"
    "\"connected\":%s,"
    "\"ip\":\"%s\","
    "\"rssi\":%d,"
    "\"hostname\":\"%s\""
    "},"
    "\"wifi_configured\":%s,"
    "\"provisioning\":%s,"
    "\"setup_ap_ssid\":\"%s\","
    "\"setup_ap_ip\":\"%s\","
    "\"oled\":%s"
    "}",
    (unsigned long)millis(),
    (unsigned)ESP.getFreeHeap(),
    (unsigned)ESP.getHeapSize(),
    tempFrag,
    wifiOk ? "true" : "false",
    wifiOk ? wifiIpText() : "",
    wifiOk ? (int)WiFi.RSSI() : 0,
    httpMdnsHostname(),
    settingsWifiConfigured() ? "true" : "false",
    wifiProvisioningMode() ? "true" : "false",
    wifiProvisioningMode() ? wifiApSsid() : "",
    wifiProvisioningMode() ? wifiApIpText() : "",
    oledAvailable ? "true" : "false"
  );

  httpSendJson(server, 200, body);
}
