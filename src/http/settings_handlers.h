#pragma once

#include <WebServer.h>
#include <WString.h>

void handleSettingsGet(WebServer& server);
void handleSettingsPost(WebServer& server);
void handleSettingsReset(WebServer& server);
bool isSettingsOrAnimPath(const String& uri);
