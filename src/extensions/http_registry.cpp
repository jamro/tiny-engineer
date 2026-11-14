#include "extensions/http_registry.h"

#include <optional>

#include "extensions/lcd1602/http.h"
#include "http/access.h"
#include "http/response.h"
#include "settings/settings.h"

namespace {

struct ExtHttpRoute {
  const char* path;
  HTTPMethod method;
  Access access;
  bool (*enabled)();
  void (*handler)(WebServer&);
};

bool lcd1602On() {
  return settingsLcd1602();
}

constexpr ExtHttpRoute kExtRoutes[] = {
  {"/extensions/lcd1602/text", HTTP_GET, Access::Configured, lcd1602On, handleLcd1602TextGet},
  {"/extensions/lcd1602/text", HTTP_POST, Access::Configured, lcd1602On, handleLcd1602TextPost},
  {"/extensions/lcd1602/text", HTTP_DELETE, Access::Configured, lcd1602On, handleLcd1602TextDelete},
  {"/extensions/lcd1602/brightness", HTTP_GET, Access::Configured, lcd1602On, handleLcd1602BrightnessGet},
  {"/extensions/lcd1602/brightness", HTTP_POST, Access::Configured, lcd1602On, handleLcd1602BrightnessPost},
};

}  // namespace

void registerExtensionHttpRoutes(WebServer& server) {
  for (const ExtHttpRoute& route : kExtRoutes) {
    // Reference into kExtRoutes keeps the capture small enough for std::function.
    server.on(route.path, route.method, [&server, &route]() {
      if (const std::optional<Refusal> refusal = Refusal::check(server, route.access)) {
        refusal->send(server);
        return;
      }

      if (!route.enabled()) {
        sendError(server, 503, "extension disabled");
        return;
      }

      route.handler(server);
    });
  }
}

bool extensionHttpPathKnown(const String& uri) {
  for (const ExtHttpRoute& route : kExtRoutes) {
    if (uri == route.path) {
      return true;
    }
  }
  return false;
}
