#include <Arduino.h>
#include <WiFi.h>

#include "display/oled.h"
#include "firmware_version.h"
#include "hardware/chip_temp.h"
#include "hardware/i2c_health.h"
#include "http/handlers.h"
#include "http/response.h"
#include "network/wifi_connect.h"
#include "settings/settings.h"

void handleAuth(WebServer& server) {
  JsonDocument body;
  body["ok"] = true;
  body["version"] = FW_VERSION;
  body["required"] = settingsAccessTokenSet();
  body["wifi_configured"] = settingsWifiConfigured();
  body["link_mode"] = settingsLinkMode();
  body["provisioning"] = wifiProvisioningMode();
  sendJson(server, 200, body);
}

void handleHealth(WebServer& server) {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const bool provisioning = wifiProvisioningMode();
  JsonDocument body;

  body["ok"] = true;
  body["version"] = FW_VERSION;
  body["uptime_ms"] = millis();
  body["free_heap"] = ESP.getFreeHeap();
  body["heap_size"] = ESP.getHeapSize();

  float tempC = 0.0f;

  if (chipTempCelsius(&tempC)) {
    body["cpu_temp_c"] = serialized(String(tempC, 1));
  } else {
    body["cpu_temp_c"] = nullptr;
  }

  JsonObject wifi = body["wifi"].to<JsonObject>();
  wifi["connected"] = connected;
  wifi["ip"] = connected ? wifiIpText() : "";
  wifi["rssi"] = connected ? WiFi.RSSI() : 0;
  wifi["hostname"] = wifiMdnsHostname();

  body["wifi_configured"] = settingsWifiConfigured();
  body["link_mode"] = settingsLinkMode();
  body["provisioning"] = provisioning;
  body["setup_ap_ssid"] = provisioning ? wifiApSsid() : "";
  body["setup_ap_ip"] = provisioning ? wifiApIpText() : "";
  body["oled"] = oledAvailable;

  const I2cHealth bus = i2cHealth();
  JsonObject i2c = body["i2c"].to<JsonObject>();
  i2c["ok"] = bus.ok;
  i2c["pca9685"] = bus.servoDriver;
  i2c["faults"] = bus.faults;
  i2c["recoveries"] = bus.recoveries;
  i2c["last_fault_ms"] = bus.lastFaultMs;
  sendJson(server, 200, body);
}
