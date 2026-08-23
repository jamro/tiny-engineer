#pragma once

#include <WebServer.h>

void httpSendJson(WebServer& server, int code, const char* body);
void httpSendHtml(WebServer& server, int code, const char* body);
void httpSendCorsPreflight(WebServer& server);

// Returns false and sends 401 when access token is configured and missing/wrong.
bool httpRequireApiAuth(WebServer& server);
