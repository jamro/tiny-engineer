#include "extensions/http_registry.h"

#include <cstdio>
#include <cstring>
#include <optional>

#include "extensions/lcd1602/api.h"
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
  {"/extensions/lcd1602/color", HTTP_GET, Access::Configured, lcd1602On, handleLcd1602ColorGet},
  {"/extensions/lcd1602/color", HTTP_POST, Access::Configured, lcd1602On, handleLcd1602ColorPost},
};

bool parseMethod(const char* method, HTTPMethod& out) {
  if (method == nullptr) {
    return false;
  }
  if (strcmp(method, "GET") == 0) {
    out = HTTP_GET;
    return true;
  }
  if (strcmp(method, "POST") == 0) {
    out = HTTP_POST;
    return true;
  }
  if (strcmp(method, "DELETE") == 0) {
    out = HTTP_DELETE;
    return true;
  }
  if (strcmp(method, "PUT") == 0) {
    out = HTTP_PUT;
    return true;
  }
  return false;
}

void setErr(char* err, size_t errLen, const char* message) {
  if (err == nullptr || errLen == 0) {
    return;
  }
  strlcpy(err, message != nullptr ? message : "error", errLen);
}

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

int extensionInvoke(
  const char* id,
  const char* suffix,
  const char* method,
  JsonVariantConst body,
  JsonDocument& out,
  char* err,
  size_t errLen
) {
  if (id == nullptr || id[0] == '\0' || suffix == nullptr || suffix[0] == '\0') {
    setErr(err, errLen, "missing id or suffix");
    return 400;
  }

  HTTPMethod httpMethod;
  if (!parseMethod(method, httpMethod)) {
    setErr(err, errLen, "invalid method");
    return 400;
  }

  char path[64];
  const int written = snprintf(path, sizeof(path), "/extensions/%s/%s", id, suffix);
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(path)) {
    setErr(err, errLen, "path too long");
    return 400;
  }

  const bool pathKnown = extensionHttpPathKnown(path);
  const ExtHttpRoute* matched = nullptr;
  for (const ExtHttpRoute& route : kExtRoutes) {
    if (strcmp(route.path, path) == 0 && route.method == httpMethod) {
      matched = &route;
      break;
    }
  }

  if (matched == nullptr) {
    if (pathKnown) {
      setErr(err, errLen, "method not allowed");
      return 405;
    }
    setErr(err, errLen, "not found");
    return 404;
  }

  // Serial already gates provisioning; Configured access without WebServer = enabled toggle.
  if (!matched->enabled()) {
    setErr(err, errLen, "extension disabled");
    return 503;
  }

  JsonObjectConst obj = body.is<JsonObjectConst>() ? body.as<JsonObjectConst>() : JsonObjectConst{};

  if (strcmp(path, "/extensions/lcd1602/text") == 0) {
    if (httpMethod == HTTP_GET) {
      return lcd1602ApiTextGet(out);
    }
    if (httpMethod == HTTP_POST) {
      return lcd1602ApiTextPost(obj, out, err, errLen);
    }
    if (httpMethod == HTTP_DELETE) {
      const int code = lcd1602ApiTextDelete(out);
      if (code != 200) {
        setErr(err, errLen, "lcd not found");
      }
      return code;
    }
  }

  if (strcmp(path, "/extensions/lcd1602/brightness") == 0) {
    if (httpMethod == HTTP_GET) {
      return lcd1602ApiBrightnessGet(out);
    }
    if (httpMethod == HTTP_POST) {
      return lcd1602ApiBrightnessPost(obj, out, err, errLen);
    }
  }

  if (strcmp(path, "/extensions/lcd1602/color") == 0) {
    if (httpMethod == HTTP_GET) {
      return lcd1602ApiColorGet(out, err, errLen);
    }
    if (httpMethod == HTTP_POST) {
      return lcd1602ApiColorPost(obj, out, err, errLen);
    }
  }

  setErr(err, errLen, "not found");
  return 404;
}
