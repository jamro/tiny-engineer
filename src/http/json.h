#pragma once

#include <WebServer.h>

void httpSendJson(WebServer& server, int code, const char* body);
void httpSendHtml(WebServer& server, int code, const char* body);
