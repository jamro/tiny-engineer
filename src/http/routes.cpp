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
#include "sleep.h"

namespace {

void handleIndex() {
  sendIndexPage(httpServer());
}

void handleNotFound() {
  WebServer& server = httpServer();

  if (server.method() == HTTP_OPTIONS) {
    httpSendCorsPreflight(server);
    return;
  }

  touchApiActivity();

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
    httpWithApiAuth(httpServer(), handleAnimGet);
  });
  server.on("/anim", HTTP_POST, []() {
    httpWithApiAuth(httpServer(), handleAnimPost);
  });
  server.on("/settings", HTTP_GET, []() {
    httpWithApiAuth(httpServer(), handleSettingsGet);
  });
  server.on("/settings", HTTP_POST, []() {
    httpWithApiAuth(httpServer(), handleSettingsPost);
  });
  registerHttpTestRoutes(server);
  server.onNotFound(handleNotFound);
}
