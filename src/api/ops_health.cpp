#include "api/ops.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstdio>

#include "display/oled.h"
#include "firmware_version.h"
#include "hardware/chip_temp.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "settings/settings.h"

void apiOpAuthGet(const ApiRequest& /*req*/, ApiResponse& resp) {
  char body[128];
  std::snprintf(
    body,
    sizeof(body),
    "{\"ok\":true,\"required\":%s,\"wifi_configured\":%s,\"provisioning\":%s}",
    settingsAccessTokenSet() ? "true" : "false",
    settingsWifiConfigured() ? "true" : "false",
    wifiProvisioningMode() ? "true" : "false"
  );
  resp.set(200, body);
}

void apiOpHealthGet(const ApiRequest& /*req*/, ApiResponse& resp) {
  const bool wifiOk = WiFi.status() == WL_CONNECTED;

  refreshMdnsHostname();

  float tempC = 0.0f;
  const bool tempOk = chipTempCelsius(&tempC);
  char tempFrag[24];
  if (tempOk) {
    std::snprintf(tempFrag, sizeof(tempFrag), "%.1f", tempC);
  } else {
    std::snprintf(tempFrag, sizeof(tempFrag), "null");
  }

  char body[768];
  std::snprintf(
    body,
    sizeof(body),
    "{"
    "\"ok\":true,"
    "\"version\":\"%s\","
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
    FW_VERSION,
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

  resp.set(200, body);
}
