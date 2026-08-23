#include "http/http_server.h"

#include <Arduino.h>

#include "http/routes.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"

void startHttpServer() {
  if (!wifiConnected() && !wifiProvisioningMode()) {
    Serial.println("HTTP: skipped (no wifi)");
    return;
  }

  WebServer& server = httpServer();

  refreshMdnsHostname();

  static const char* kCollectHeaders[] = {"Authorization"};
  server.collectHeaders(kCollectHeaders, 1);

  registerHttpRoutes();
  server.begin();

  if (wifiProvisioningMode()) {
    Serial.print("HTTP setup: http://");
    Serial.print(wifiApIpText());
    Serial.println("/config");
    return;
  }

  Serial.print("HTTP: http://");
  Serial.print(wifiIpText());
  Serial.println("/");
  Serial.print("HTTP: http://");
  Serial.print(httpMdnsHostname());
  Serial.println("/");
  Serial.print("HTTP: http://");
  Serial.print(wifiIpText());
  Serial.println("/health");
  Serial.print("HTTP: http://");
  Serial.print(httpMdnsHostname());
  Serial.println("/health");
}

void pollHttpServer() {
  if (!wifiConnected() && !wifiProvisioningMode()) {
    delay(10);
    return;
  }

  httpServer().handleClient();
}
