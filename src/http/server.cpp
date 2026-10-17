#include "http/server.h"

#include <Arduino.h>
#include <Middlewares.h>
#include <WebServer.h>

#include "http/routes.h"
#include "network/wifi_connect.h"
#include "serial_log.h"

namespace {

WebServer server(80);
CorsMiddleware cors;
bool httpStarted = false;

bool serving() {
  return wifiConnected() || wifiProvisioningMode();
}

void logUrl(const String& host, const char* path) {
  serialLogPrintln(String("HTTP: http://") + host + path);
}

}  // namespace

void startHttpServer() {
  if (!serving()) {
    serialLogPrintln("HTTP: skipped (no wifi)");
    return;
  }

  // Origin drives CorsMiddleware; Authorization is the bearer token.
  static const char* kCollectHeaders[] = {"Authorization", "Origin"};
  server.collectHeaders(kCollectHeaders, 2);

  cors.setOrigin("*")
    .setMethods("GET, POST, PUT, OPTIONS")
    .setHeaders("Authorization, Content-Type")
    .setAllowCredentials(false);
  server.addMiddleware(&cors);

  registerHttpRoutes(server);
  server.begin();
  httpStarted = true;

  if (wifiProvisioningMode()) {
    logUrl(wifiApIpText(), "/config");
    return;
  }

  const String mdnsHost = wifiMdnsHostname();

  for (const char* path : {"/", "/health"}) {
    logUrl(wifiIpText(), path);
    logUrl(mdnsHost, path);
  }
}

void stopHttpServer() {
  if (!httpStarted) {
    return;
  }
  server.close();
  httpStarted = false;
  serialLogPrintln("HTTP: stopped");
}

void pollHttpServer() {
  if (!httpStarted || !serving()) {
    delay(10);
    return;
  }

  server.handleClient();
}
