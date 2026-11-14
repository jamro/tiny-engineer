#pragma once

#include <WebServer.h>

void registerExtensionHttpRoutes(WebServer& server);
bool extensionHttpPathKnown(const String& uri);
