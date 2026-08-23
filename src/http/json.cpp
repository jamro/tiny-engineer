#include "http/json.h"

#include <cstring>

#include "settings.h"

namespace {

void sendCorsHeaders(WebServer& server) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Authorization");
  server.sendHeader(
    "Access-Control-Allow-Methods",
    "GET, POST, OPTIONS"
  );
}

void sendWithCors(WebServer& server, int code, const char* type, const char* body) {
  sendCorsHeaders(server);
  server.send(code, type, body);
}

bool bearerTokenMatches(const String& header, const char* expected) {
  constexpr const char* kPrefix = "Bearer ";
  constexpr size_t kPrefixLen = 7;

  if (header.length() < kPrefixLen) {
    return false;
  }

  for (size_t i = 0; i < kPrefixLen; i++) {
    if (header[i] != kPrefix[i]) {
      return false;
    }
  }

  const char* presented = header.c_str() + kPrefixLen;
  return strcmp(presented, expected) == 0;
}

}  // namespace

void httpSendJson(WebServer& server, int code, const char* body) {
  sendWithCors(server, code, "application/json", body);
}

void httpSendHtml(WebServer& server, int code, const char* body) {
  sendWithCors(server, code, "text/html; charset=utf-8", body);
}

void httpSendCorsPreflight(WebServer& server) {
  sendCorsHeaders(server);
  server.send(204);
}

bool httpRequireApiAuth(WebServer& server) {
  if (!settingsAccessTokenSet()) {
    return true;
  }

  if (!server.hasHeader("Authorization") ||
      !bearerTokenMatches(
        server.header("Authorization"),
        settingsAccessToken()
      )) {
    httpSendJson(
      server,
      401,
      "{\"ok\":false,\"error\":\"unauthorized\"}"
    );
    return false;
  }

  return true;
}

bool httpRequireWifiConfigured(WebServer& server) {
  if (settingsWifiConfigured()) {
    return true;
  }

  httpSendJson(
    server,
    503,
    "{\"ok\":false,\"error\":\"wifi not configured\"}"
  );
  return false;
}

void httpWithApiAuth(WebServer& server, void (*handler)(WebServer&)) {
  if (!httpRequireApiAuth(server)) {
    return;
  }

  handler(server);
}

void httpWithWifiAndApiAuth(WebServer& server, void (*handler)(WebServer&)) {
  if (!httpRequireWifiConfigured(server)) {
    return;
  }

  httpWithApiAuth(server, handler);
}
