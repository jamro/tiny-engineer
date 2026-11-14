#pragma once

#include <WebServer.h>

void handleLcd1602TextGet(WebServer& server);
void handleLcd1602TextPost(WebServer& server);
void handleLcd1602TextDelete(WebServer& server);
void handleLcd1602BrightnessGet(WebServer& server);
void handleLcd1602BrightnessPost(WebServer& server);
