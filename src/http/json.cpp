#include "http/json.h"

#include <cstring>

#include "settings/settings.h"

namespace {

void sendCorsHeaders(WebServer& server) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader(
    "Access-Control-Allow-Headers",
    "Authorization, Content-Type"
  );
  server.sendHeader(
    "Access-Control-Allow-Methods",
    "GET, POST, PUT, OPTIONS"
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
  sendCorsHeaders(server);
  // send() copies into Arduino String and fails on the large panel.
  server.send_P(code, "text/html; charset=utf-8", body);
}

void httpSendCorsPreflight(WebServer& server) {
  sendCorsHeaders(server);
  server.send(204);
}

bool httpApiAuthorized(WebServer& server) {
  return !settingsAccessTokenSet() ||
         (server.hasHeader("Authorization") &&
          bearerTokenMatches(server.header("Authorization"), settingsAccessToken()));
}

bool httpRequireApiAuth(WebServer& server) {
  if (!httpApiAuthorized(server)) {
    httpSendJson(
      server,
      401,
      "{\"ok\":false,\"error\":\"unauthorized\"}"
    );
    return false;
  }

  return true;
}

void httpWithApiAuth(WebServer& server, void (*handler)(WebServer&)) {
  if (!httpRequireApiAuth(server)) {
    return;
  }

  handler(server);
}
