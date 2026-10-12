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
  std::snprintf(
    resp.body,
    ApiResponse::kBodyMax,
    "{\"ok\":true,\"required\":%s,\"wifi_configured\":%s,\"provisioning\":%s}",
    settingsAccessTokenSet() ? "true" : "false",
    settingsWifiConfigured() ? "true" : "false",
    wifiProvisioningMode() ? "true" : "false"
  );
  resp.status = 200;
}

void apiOpHealthGet(const ApiRequest& /*req*/, ApiResponse& resp) {
  const bool wifiOk = WiFi.status() == WL_CONNECTED;

  refreshMdnsHostname();

  float tempC = 0.0f;
  const bool tempOk = chipTempCelsius(&tempC);
  // Integer tenths avoids newlib dtoa (heavy stack) on loopTask.
  char tempFrag[24];
  if (tempOk) {
    const int tenths = static_cast<int>(tempC * 10.0f + 0.5f);
    std::snprintf(tempFrag, sizeof(tempFrag), "%d.%d", tenths / 10, tenths % 10);
  } else {
    std::snprintf(tempFrag, sizeof(tempFrag), "null");
  }

  std::snprintf(
    resp.body,
    ApiResponse::kBodyMax,
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
  resp.status = 200;
}
