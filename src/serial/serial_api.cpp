#include "serial/serial_api.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

#include "api/dispatch.h"
#include "api/request.h"

namespace {

constexpr size_t kLineMax = 1024;

char lineBuf[kLineMax];
size_t lineLen = 0;

void writeReply(unsigned long id, const ApiResponse& resp) {
  // R{"v":1,"id":N,"status":S,"body":...}
  // body is already a JSON object string — embed without re-escaping.
  Serial.print('R');
  Serial.print("{\"v\":1,\"id\":");
  Serial.print(id);
  Serial.print(",\"status\":");
  Serial.print(resp.status);
  Serial.print(",\"body\":");
  if (resp.body[0] != '\0') {
    Serial.print(resp.body);
  } else {
    Serial.print("null");
  }
  Serial.println('}');
}

void rejectBadRequest(unsigned long id, const char* error) {
  ApiResponse resp;
  resp.setError(400, error);
  writeReply(id, resp);
}

void handleLine(char* line) {
  // Trim trailing CR
  size_t n = std::strlen(line);
  while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) {
    line[--n] = '\0';
  }

  if (n == 0) {
    return;
  }

  // Only JSON objects are requests; ignore noise / monitor typing.
  if (line[0] != '{') {
    return;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, line);
  if (err) {
    rejectBadRequest(0, "invalid json");
    return;
  }

  const unsigned long id = doc["id"] | 0UL;

  if (!doc["v"].is<int>() || doc["v"].as<int>() != 1) {
    rejectBadRequest(id, "unsupported version");
    return;
  }

  if (!doc["op"].is<const char*>()) {
    rejectBadRequest(id, "missing op");
    return;
  }

  const char* op = doc["op"];
  ApiRequest req;
  req.clear();

  if (!apiParseOp(op, req.method, req.path, sizeof(req.path))) {
    rejectBadRequest(id, "invalid op");
    return;
  }

  if (doc["q"].is<JsonObjectConst>()) {
    for (JsonPairConst kv : doc["q"].as<JsonObjectConst>()) {
      // Protocol: q values are strings (same as HTTP query params).
      char tmp[ApiRequest::kValueMax];
      const char* val = nullptr;

      if (kv.value().is<const char*>()) {
        val = kv.value().as<const char*>();
      } else if (kv.value().is<bool>()) {
        val = kv.value().as<bool>() ? "1" : "0";
      } else if (kv.value().is<long>() || kv.value().is<unsigned long>() ||
                 kv.value().is<int>() || kv.value().is<float>() ||
                 kv.value().is<double>()) {
        serializeJson(kv.value(), tmp, sizeof(tmp));
        val = tmp;
      } else {
        rejectBadRequest(id, "q values must be strings or numbers");
        return;
      }

      if (!req.addQuery(kv.key().c_str(), val)) {
        rejectBadRequest(id, "query too large");
        return;
      }
    }
  } else if (!doc["q"].isNull() && !doc["q"].is<JsonObjectConst>()) {
    rejectBadRequest(id, "q must be an object");
    return;
  }

  ApiResponse resp;
  apiDispatch(req, resp);
  writeReply(id, resp);
}

}  // namespace

void pollSerialApi() {
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      break;
    }

    if (ch == '\n') {
      lineBuf[lineLen] = '\0';
      handleLine(lineBuf);
      lineLen = 0;
      continue;
    }

    if (ch == '\r') {
      continue;
    }

    if (lineLen + 1 >= kLineMax) {
      // Overflow: discard until newline, then 400.
      lineLen = 0;
      while (Serial.available() > 0) {
        const int skip = Serial.read();
        if (skip == '\n' || skip < 0) {
          break;
        }
      }
      rejectBadRequest(0, "line too long");
      continue;
    }

    lineBuf[lineLen++] = static_cast<char>(ch);
  }
}
