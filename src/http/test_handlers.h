#pragma once

#include <WebServer.h>
#include <WString.h>

void registerHttpTestRoutes(WebServer& server);
bool isHttpTestPath(const String& uri);
