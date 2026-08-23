#pragma once

#include <WebServer.h>

void httpSendJson(WebServer& server, int code, const char* body);
void httpSendHtml(WebServer& server, int code, const char* body);
void httpSendCorsPreflight(WebServer& server);

// Returns false and sends 401 when access token is configured and missing/wrong.
bool httpRequireApiAuth(WebServer& server);

// Returns false and sends 503 when WiFi credentials are not saved yet.
bool httpRequireWifiConfigured(WebServer& server);

// WiFi configured + auth + touchApiActivity, then call handler.
void httpWithWifiAndApiAuth(WebServer& server, void (*handler)(WebServer&));

// Auth + touchApiActivity, then call handler. No-op if auth fails.
void httpWithApiAuth(WebServer& server, void (*handler)(WebServer&));
