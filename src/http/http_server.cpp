#include "http/http_server.h"

#include <Arduino.h>

#include "http/routes.h"
#include "http/server_context.h"
#include "network/wifi_connect.h"
#include "serial_log.h"

void startHttpServer() {
  if (!wifiConnected() && !wifiProvisioningMode()) {
    serialLogPrintln("HTTP: skipped (no wifi)");
    return;
  }

  WebServer& server = httpServer();

  refreshMdnsHostname();

  static const char* kCollectHeaders[] = {"Authorization"};
  server.collectHeaders(kCollectHeaders, 1);

  registerHttpRoutes();
  server.begin();

  if (wifiProvisioningMode()) {
    serialLogPrint("HTTP setup: http://");
    serialLogPrint(wifiApIpText());
    serialLogPrintln("/config");
    return;
  }

  serialLogPrint("HTTP: http://");
  serialLogPrint(wifiIpText());
  serialLogPrintln("/");
  serialLogPrint("HTTP: http://");
  serialLogPrint(httpMdnsHostname());
  serialLogPrintln("/");
  serialLogPrint("HTTP: http://");
  serialLogPrint(wifiIpText());
  serialLogPrintln("/health");
  serialLogPrint("HTTP: http://");
  serialLogPrint(httpMdnsHostname());
  serialLogPrintln("/health");
}

void pollHttpServer() {
  if (!wifiConnected() && !wifiProvisioningMode()) {
    delay(10);
    return;
  }

  httpServer().handleClient();
}
