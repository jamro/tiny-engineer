#include "extensions/lcd1602/api.h"

#include <Arduino.h>
#include <cstring>

#include "extensions/lcd1602/lcd1602.h"
#include "settings/settings.h"

namespace {

void fillTextJson(JsonDocument& doc) {
  const Lcd1602TextState& state = lcd1602TextState();
  JsonObject line1 = doc["line1"].to<JsonObject>();
  line1["text"] = state.line1.text;
  line1["align"] = lcd1602AlignName(state.line1.align);
  JsonObject line2 = doc["line2"].to<JsonObject>();
  line2["text"] = state.line2.text;
  line2["align"] = lcd1602AlignName(state.line2.align);
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

void setErr(char* err, size_t errLen, const char* message) {
  if (err == nullptr || errLen == 0) {
    return;
  }
  strlcpy(err, message != nullptr ? message : "error", errLen);
}

}  // namespace

int lcd1602ApiTextGet(JsonDocument& out) {
  fillTextJson(out);
  return 200;
}

int lcd1602ApiTextPost(JsonObjectConst body, JsonDocument& out, char* err, size_t errLen) {
  Lcd1602LineState line1{};
  Lcd1602LineState line2{};
  const Lcd1602LineState* p1 = nullptr;
  const Lcd1602LineState* p2 = nullptr;
  const char* errorField = nullptr;

  if (!body["line1"].isUnbound()) {
    if (!parseLineObject(body["line1"], line1, errorField)) {
      setErr(err, errLen, (String("invalid ") + (errorField != nullptr ? errorField : "line1")).c_str());
      return 400;
    }
    p1 = &line1;
  }

  if (!body["line2"].isUnbound()) {
    if (!parseLineObject(body["line2"], line2, errorField)) {
      setErr(err, errLen, (String("invalid ") + (errorField != nullptr ? errorField : "line2")).c_str());
      return 400;
    }
    p2 = &line2;
  }

  if (p1 == nullptr && p2 == nullptr) {
    setErr(err, errLen, "send at least one line");
    return 400;
  }

  for (JsonPairConst field : body) {
    if (field.key() != "line1" && field.key() != "line2") {
      setErr(err, errLen, (String("unknown field ") + field.key().c_str()).c_str());
      return 400;
    }
  }

  if (!lcd1602SetText(p1, p2)) {
    setErr(err, errLen, "lcd not found");
    return 503;
  }

  fillTextJson(out);
  return 200;
}

int lcd1602ApiTextDelete(JsonDocument& out) {
  if (!lcd1602ClearText()) {
    // Caller maps 503 without body message; HTTP wrapper sets "lcd not found".
    return 503;
  }
  fillTextJson(out);
  return 200;
}

int lcd1602ApiBrightnessGet(JsonDocument& out) {
  out["brightness"] = lcd1602Brightness();
  return 200;
}

int lcd1602ApiBrightnessPost(JsonObjectConst body, JsonDocument& out, char* err, size_t errLen) {
  if (body["brightness"].isUnbound() || !body["brightness"].is<uint8_t>()) {
    setErr(err, errLen, "invalid brightness");
    return 400;
  }

  const uint8_t value = body["brightness"].as<uint8_t>();
  if (value > 100) {
    setErr(err, errLen, "invalid brightness");
    return 400;
  }

  for (JsonPairConst field : body) {
    if (field.key() != "brightness") {
      setErr(err, errLen, (String("unknown field ") + field.key().c_str()).c_str());
      return 400;
    }
  }

  if (!lcd1602SetBrightness(value)) {
    setErr(err, errLen, "backlight not found");
    return 503;
  }

  out["brightness"] = lcd1602Brightness();
  return 200;
}
