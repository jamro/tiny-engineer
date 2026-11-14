#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>
#include <cstddef>

void registerExtensionHttpRoutes(WebServer& server);
bool extensionHttpPathKnown(const String& uri);

// Serial / non-HTTP invoke of a registered extension route.
// Returns HTTP-like status; fills out on success and err on failure.
int extensionInvoke(
  const char* id,
  const char* suffix,
  const char* method,
  JsonVariantConst body,
  JsonDocument& out,
  char* err,
  size_t errLen
);
