#include "http/test_handlers.h"

#include "api/http_bridge.h"
#include "http/json.h"

bool isHttpTestPath(const String& uri) {
  return uri == "/test/audio" ||
    uri == "/test/audio/bell" ||
    uri == "/test/screen" ||
    uri == "/test/movement" ||
    uri == "/test/led" ||
    uri == "/test/servo";
}

void registerHttpTestRoutes(WebServer& server) {
  server.on(
    "/test/audio",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/test/audio/bell",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/test/screen",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/test/movement",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/test/led",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
  server.on(
    "/test/servo",
    HTTP_POST,
    [&server]() { httpWithWifiAndApiAuth(server, httpDispatchApi); }
  );
}
