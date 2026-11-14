#include "extensions/lcd1602/http.h"

#include <ArduinoJson.h>

#include "extensions/lcd1602/api.h"
#include "http/request.h"
#include "http/response.h"

void handleLcd1602TextGet(WebServer& server) {
  JsonDocument doc;
  lcd1602ApiTextGet(doc);
  sendJson(server, 200, doc);
}

void handleLcd1602TextPost(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody(server, body)) {
    return;
  }

  JsonDocument out;
  char err[96];
  err[0] = '\0';
  const int code = lcd1602ApiTextPost(body.as<JsonObjectConst>(), out, err, sizeof(err));
  if (code != 200) {
    sendError(server, code, err[0] != '\0' ? err : "error");
    return;
  }
  sendJson(server, 200, out);
}

void handleLcd1602TextDelete(WebServer& server) {
  JsonDocument out;
  const int code = lcd1602ApiTextDelete(out);
  if (code != 200) {
    sendError(server, code, "lcd not found");
    return;
  }
  sendJson(server, 200, out);
}

void handleLcd1602BrightnessGet(WebServer& server) {
  JsonDocument doc;
  lcd1602ApiBrightnessGet(doc);
  sendJson(server, 200, doc);
}

void handleLcd1602BrightnessPost(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody(server, body)) {
    return;
  }

  JsonDocument out;
  char err[96];
  err[0] = '\0';
  const int code = lcd1602ApiBrightnessPost(body.as<JsonObjectConst>(), out, err, sizeof(err));
  if (code != 200) {
    sendError(server, code, err[0] != '\0' ? err : "error");
    return;
  }
  sendJson(server, 200, out);
}
