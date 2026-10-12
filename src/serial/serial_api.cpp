#include "serial/serial_api.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "api/dispatch.h"
#include "api/request.h"
#include "audio/play_session.h"
#include "serial_log.h"

namespace {

constexpr size_t kLineMax = 1024;
constexpr uint32_t kPlayMaxBytes = 32u * 1024u * 1024u;
constexpr size_t kBinaryChunk = 256;
// Soft ring between USB CDC and I2S (I2S.write blocks ≈ realtime).
constexpr size_t kRxRingSize = 8192;
// Feed at most this much PCM path per poll (~45 ms @ 22050 stereo path).
constexpr size_t kMaxFeedPerPoll = 2048;

char lineBuf[kLineMax];
size_t lineLen = 0;

// Keep large buffers off the ~8KB loopTask stack (stack protection faults).
ApiRequest g_req;
ApiResponse g_resp;
uint8_t g_binaryChunk[kBinaryChunk];
uint8_t g_rxRing[kRxRingSize];
size_t g_rxHead = 0;
size_t g_rxCount = 0;

enum class Mode : uint8_t { Line, BinaryPlay };

Mode mode = Mode::Line;
unsigned long binaryId = 0;
uint32_t binaryRemaining = 0;  // body bytes not yet fed/discarded
bool binaryDiscard = false;
PlaySession playSession;

void rxRingClear() {
  g_rxHead = 0;
  g_rxCount = 0;
}

void rxRingPushFromSerial() {
  while (g_rxCount < kRxRingSize && binaryRemaining > g_rxCount && Serial.available() > 0) {
    const size_t need = static_cast<size_t>(binaryRemaining) - g_rxCount;
    size_t space = kRxRingSize - g_rxCount;
    if (space > need) {
      space = need;
    }
    const size_t writeAt = (g_rxHead + g_rxCount) % kRxRingSize;
    size_t contig = kRxRingSize - writeAt;
    if (contig > space) {
      contig = space;
    }
    int avail = Serial.available();
    if (avail <= 0) {
      break;
    }
    size_t want = contig;
    if (static_cast<size_t>(avail) < want) {
      want = static_cast<size_t>(avail);
    }
    const int got =
        Serial.readBytes(reinterpret_cast<char*>(g_rxRing + writeAt), want);
    if (got <= 0) {
      break;
    }
    g_rxCount += static_cast<size_t>(got);
  }
}

size_t rxRingPop(uint8_t* dst, size_t n) {
  const size_t take = std::min(n, g_rxCount);
  for (size_t i = 0; i < take; i++) {
    dst[i] = g_rxRing[g_rxHead];
    g_rxHead = (g_rxHead + 1) % kRxRingSize;
  }
  g_rxCount -= take;
  return take;
}

void writeReply(unsigned long id, int status, const char* bodyJson) {
  Serial.print('R');
  Serial.print("{\"v\":1,\"id\":");
  Serial.print(id);
  Serial.print(",\"status\":");
  Serial.print(status);
  Serial.print(",\"body\":");
  if (bodyJson != nullptr && bodyJson[0] != '\0') {
    Serial.print(bodyJson);
  } else {
    Serial.print("null");
  }
  Serial.println('}');
}

void writeReply(unsigned long id, const ApiResponse& resp) {
  writeReply(id, resp.status, resp.body);
}

void rejectBadRequest(unsigned long id, const char* error) {
  g_resp.setError(400, error);
  writeReply(id, g_resp);
}

void finishBinaryPlay() {
  playSession.end();
  writeReply(binaryId, playSession.statusCode(), playSession.bodyJson());
  setSerialBinaryBusy(false);
  mode = Mode::Line;
  binaryRemaining = 0;
  binaryDiscard = false;
  rxRingClear();
}

void startBinaryPlay(unsigned long id, uint32_t len, const char* animName) {
  binaryId = id;
  binaryRemaining = len;
  rxRingClear();
  playSession.begin(animName);
  binaryDiscard = playSession.rejected();
  setSerialBinaryBusy(true);
  mode = Mode::BinaryPlay;

  if (binaryRemaining == 0) {
    finishBinaryPlay();
  }
}

void pollBinaryPlay() {
  // Pull USB into ring first (no I2S wait).
  rxRingPushFromSerial();

  size_t fed = 0;
  while (binaryRemaining > 0 && fed < kMaxFeedPerPoll) {
    rxRingPushFromSerial();
    if (g_rxCount == 0) {
      break;
    }

    size_t want = kBinaryChunk;
    if (want > binaryRemaining) {
      want = binaryRemaining;
    }
    if (want > g_rxCount) {
      want = g_rxCount;
    }

    const size_t got = rxRingPop(g_binaryChunk, want);
    if (got == 0) {
      break;
    }

    if (!binaryDiscard) {
      // May block in I2S ≈ realtime for these samples.
      playSession.feed(g_binaryChunk, got);
    }
    binaryRemaining -= static_cast<uint32_t>(got);
    fed += got;

    // After possible I2S block, suck CDC again before next slice.
    rxRingPushFromSerial();
  }

  if (binaryRemaining == 0) {
    finishBinaryPlay();
  }
}

void handleLine(char* line) {
  size_t n = std::strlen(line);
  while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) {
    line[--n] = '\0';
  }

  if (n == 0) {
    return;
  }

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
  g_req.clear();

  if (!apiParseOp(op, g_req.method, g_req.path, sizeof(g_req.path))) {
    rejectBadRequest(id, "invalid op");
    return;
  }

  if (doc["q"].is<JsonObjectConst>()) {
    for (JsonPairConst kv : doc["q"].as<JsonObjectConst>()) {
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

      if (!g_req.addQuery(kv.key().c_str(), val)) {
        rejectBadRequest(id, "query too large");
        return;
      }
    }
  } else if (!doc["q"].isNull() && !doc["q"].is<JsonObjectConst>()) {
    rejectBadRequest(id, "q must be an object");
    return;
  }

  // POST /play[/<name>] with len → binary WAV body (not apiDispatch).
  if (g_req.method == ApiMethod::Post && isPlayApiPath(g_req.path)) {
    if (!doc["len"].is<uint32_t>() && !doc["len"].is<int>() && !doc["len"].is<long>() &&
        !doc["len"].is<unsigned long>()) {
      rejectBadRequest(id, "play requires len");
      return;
    }
    const long long lenLl = doc["len"].as<long long>();
    if (lenLl <= 0 || static_cast<unsigned long long>(lenLl) > kPlayMaxBytes) {
      rejectBadRequest(id, "invalid play len");
      return;
    }
    const char* animName = playAnimNameFromPath(g_req.path);
    startBinaryPlay(id, static_cast<uint32_t>(lenLl), animName);
    return;
  }

  apiDispatch(g_req, g_resp);
  writeReply(id, g_resp);
}

}  // namespace

void pollSerialApi() {
  if (mode == Mode::BinaryPlay) {
    pollBinaryPlay();
    return;
  }

  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      break;
    }

    if (ch == '\n') {
      lineBuf[lineLen] = '\0';
      handleLine(lineBuf);
      lineLen = 0;
      if (mode == Mode::BinaryPlay) {
        pollBinaryPlay();
        return;
      }
      continue;
    }

    if (ch == '\r') {
      continue;
    }

    if (lineLen + 1 >= kLineMax) {
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
