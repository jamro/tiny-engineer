#include "http/json.h"

namespace {

void sendWithCors(WebServer& server, int code, const char* type, const char* body) {
  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );
  server.send(
    code,
    type,
    body
  );
}

}  // namespace

void httpSendJson(WebServer& server, int code, const char* body) {
  sendWithCors(server, code, "application/json", body);
}

void httpSendHtml(WebServer& server, int code, const char* body) {
  sendWithCors(server, code, "text/html; charset=utf-8", body);
}
