#include "extensions/lcd1602/http.h"

#include <ArduinoJson.h>
#include <cstring>

#include "extensions/lcd1602/lcd1602.h"
#include "http/request.h"
#include "http/response.h"
#include "settings/settings.h"

namespace {

JsonDocument textJson() {
  JsonDocument doc;
  const Lcd1602TextState& state = lcd1602TextState();
  JsonObject line1 = doc["line1"].to<JsonObject>();
  line1["text"] = state.line1.text;
  line1["align"] = lcd1602AlignName(state.line1.align);
  JsonObject line2 = doc["line2"].to<JsonObject>();
  line2["text"] = state.line2.text;
  line2["align"] = lcd1602AlignName(state.line2.align);
  return doc;
}

bool parseLineObject(JsonVariantConst value, Lcd1602LineState& out, const char*& errorField) {
  if (!value.is<JsonObjectConst>()) {
    errorField = "line";
    return false;
  }

  JsonObjectConst obj = value.as<JsonObjectConst>();
  if (obj["text"].isUnbound() || !obj["text"].is<const char*>()) {
    errorField = "text";
    return false;
  }

  const char* text = obj["text"].as<const char*>();
  if (text == nullptr || !settingsValidateLcd1602Line(text)) {
    errorField = "text";
    return false;
  }

  bool alignOk = true;
  Lcd1602Align align = Lcd1602Align::Left;
  if (!obj["align"].isUnbound()) {
    if (!obj["align"].is<const char*>()) {
      errorField = "align";
      return false;
    }
    align = lcd1602ParseAlign(obj["align"].as<const char*>(), &alignOk);
    if (!alignOk) {
      errorField = "align";
      return false;
    }
  }

  strlcpy(out.text, text, sizeof(out.text));
  out.align = align;
  return true;
}

}  // namespace

void handleLcd1602TextGet(WebServer& server) {
  sendJson(server, 200, textJson());
}

void handleLcd1602TextPost(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody(server, body)) {
    return;
  }

  JsonObjectConst obj = body.as<JsonObjectConst>();
  Lcd1602LineState line1{};
  Lcd1602LineState line2{};
  const Lcd1602LineState* p1 = nullptr;
  const Lcd1602LineState* p2 = nullptr;
  const char* errorField = nullptr;

  if (!obj["line1"].isUnbound()) {
    if (!parseLineObject(obj["line1"], line1, errorField)) {
      sendError(server, 400, (String("invalid ") + (errorField != nullptr ? errorField : "line1")).c_str());
      return;
    }
    p1 = &line1;
  }

  if (!obj["line2"].isUnbound()) {
    if (!parseLineObject(obj["line2"], line2, errorField)) {
      sendError(server, 400, (String("invalid ") + (errorField != nullptr ? errorField : "line2")).c_str());
      return;
    }
    p2 = &line2;
  }

  if (p1 == nullptr && p2 == nullptr) {
    sendError(server, 400, "send at least one line");
    return;
  }

  for (JsonPairConst field : obj) {
    if (field.key() != "line1" && field.key() != "line2") {
      sendError(server, 400, (String("unknown field ") + field.key().c_str()).c_str());
      return;
    }
  }

  if (!lcd1602SetText(p1, p2)) {
    sendError(server, 503, "lcd not found");
    return;
  }

  sendJson(server, 200, textJson());
}

void handleLcd1602TextDelete(WebServer& server) {
  if (!lcd1602ClearText()) {
    sendError(server, 503, "lcd not found");
    return;
  }

  sendJson(server, 200, textJson());
}

void handleLcd1602BrightnessGet(WebServer& server) {
  JsonDocument doc;
  doc["brightness"] = lcd1602Brightness();
  sendJson(server, 200, doc);
}

void handleLcd1602BrightnessPost(WebServer& server) {
  JsonDocument body;
  if (!readJsonBody(server, body)) {
    return;
  }

  JsonObjectConst obj = body.as<JsonObjectConst>();
  if (obj["brightness"].isUnbound() || !obj["brightness"].is<uint8_t>()) {
    sendError(server, 400, "invalid brightness");
    return;
  }

  const uint8_t value = obj["brightness"].as<uint8_t>();
  if (value > 100) {
    sendError(server, 400, "invalid brightness");
    return;
  }

  for (JsonPairConst field : obj) {
    if (field.key() != "brightness") {
      sendError(server, 400, (String("unknown field ") + field.key().c_str()).c_str());
      return;
    }
  }

  if (!lcd1602SetBrightness(value)) {
    sendError(server, 503, "backlight not found");
    return;
  }

  JsonDocument doc;
  doc["brightness"] = lcd1602Brightness();
  sendJson(server, 200, doc);
}
