#pragma once

#include <WebServer.h>
#include <WString.h>

void registerHttpSetupRoutes(WebServer& server);
bool isHttpSetupPath(const String& uri);
