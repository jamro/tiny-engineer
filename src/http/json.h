#pragma once

#include <WebServer.h>

void httpSendJson(WebServer& server, int code, const char* body);
