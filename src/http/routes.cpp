#include "http/routes.h"

#include <Arduino.h>
#include <WebServer.h>

#include "http/anim_handlers.h"
#include "http/health_handlers.h"
#include "http/index_page.h"
#include "http/json.h"
#include "http/server_context.h"
#include "http/settings_handlers.h"
#include "http/test_handlers.h"
#include "network/wifi_connect.h"

namespace {

void handleIndex() {
  WebServer& server = httpServer();

  if (wifiProvisioningMode() && server.uri() == "/") {
    server.sendHeader(
      "Location",
      String("http://") + wifiApIpText() + "/config",
      true
    );
    server.send(302, "text/plain", "Redirecting to setup");
    return;
  }

  sendIndexPage(httpServer());
}

void handleNotFound() {
  WebServer& server = httpServer();

  if (server.method() == HTTP_OPTIONS) {
    httpSendCorsPreflight(server);
    return;
  }

  if (wifiProvisioningMode() && server.method() == HTTP_GET) {
    server.sendHeader("Location", String("http://") + wifiApIpText() + "/config", true);
    server.send(302, "text/plain", "Redirecting to setup");
    return;
  }

  if (isHttpTestPath(server.uri()) || isSettingsOrAnimPath(server.uri())) {
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

void registerHttpRoutes() {
  WebServer& server = httpServer();

  server.on("/", HTTP_GET, handleIndex);
  server.on("/animations", HTTP_GET, handleIndex);
  server.on("/servo", HTTP_GET, handleIndex);
  server.on("/tests", HTTP_GET, handleIndex);
  server.on("/api", HTTP_GET, handleIndex);
  server.on("/config", HTTP_GET, handleIndex);
  server.on("/auth", HTTP_GET, handleAuth);
  server.on("/health", HTTP_GET, []() {
    httpWithApiAuth(httpServer(), handleHealth);
  });
  server.on("/anim", HTTP_GET, []() {
    httpWithWifiAndApiAuth(httpServer(), handleAnimGet);
  });
  server.on("/anim", HTTP_POST, []() {
    httpWithWifiAndApiAuth(httpServer(), handleAnimPost);
  });
  server.on("/settings", HTTP_GET, []() {
    httpWithApiAuth(httpServer(), handleSettingsGet);
  });
  server.on("/settings", HTTP_POST, []() {
    httpWithApiAuth(httpServer(), handleSettingsPost);
  });
  server.on("/settings/reset", HTTP_POST, []() {
    httpWithApiAuth(httpServer(), handleSettingsReset);
  });
  registerHttpTestRoutes(server);
  server.onNotFound(handleNotFound);
}
