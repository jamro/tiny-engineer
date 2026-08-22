#include "http/json.h"

void httpSendJson(WebServer& server, int code, const char* body) {
  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );
  server.send(
    code,
    "application/json",
    body
  );
}
