#pragma once

#include <cstddef>

bool wifiConnected();
bool wifiProvisioningMode();
const char* wifiIpText();
const char* wifiApSsid();
const char* wifiApIpText();
void runWifiSetup();
void pollWifi();
bool wifiTestCredentials(const char* ssid, const char* password);
const char* wifiLastConnectError();
void wifiStopProvisioningAp();
