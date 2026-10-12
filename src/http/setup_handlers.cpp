#include "http/setup_handlers.h"

#include "api/http_bridge.h"
#include "http/json.h"

bool isHttpSetupPath(const String& uri) {
  return uri == "/setup/servo" || uri == "/setup/led" ||
         uri == "/setup/audio" || uri == "/setup/oled";
}

void registerHttpSetupRoutes(WebServer& server) {
  server.on(
    "/setup/servo",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/setup/led",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/setup/audio",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/setup/oled",
    HTTP_POST,
    [&server]() { httpWithApiAuth(server, httpDispatchApi); }
  );
}
