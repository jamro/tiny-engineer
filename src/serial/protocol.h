#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint8_t kSerialProtoVersion = 1;

enum class SerialMsgType : uint8_t {
  Req = 1,
  Res = 2,
  Log = 3,
  Evt = 4,
  Stream = 5,
  Sync = 6,
};

constexpr size_t kSerialHeaderSize = 7;

struct SerialHeader {
  SerialMsgType type = SerialMsgType::Req;
  uint8_t ver = kSerialProtoVersion;
  uint16_t id = 0;
  uint8_t flags = 0;
  uint16_t len = 0;
};

// Pack header + payload into |out|. Returns total bytes, or 0 on failure.
size_t serialPackMessage(
  const SerialHeader& header,
  const uint8_t* payload,
  size_t payloadLen,
  uint8_t* out,
  size_t outMax
);

// Parse header from |data|. Returns false if truncated or len exceeds remaining bytes.
bool serialParseHeader(const uint8_t* data, size_t length, SerialHeader& out, size_t& headerBytes);

// Build a SYNC response JSON payload (null-terminated in |out|). Returns byte length
// without the trailing NUL, or 0 on failure.
size_t serialFormatSyncJson(bool provisioning, const char* linkMode, char* out, size_t outMax);

// Build a LOG JSON payload {"t":"..."}. Escapes quotes/backslashes in |text|.
size_t serialFormatLogJson(const char* text, char* out, size_t outMax);
