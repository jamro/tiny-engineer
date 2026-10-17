#include "serial/protocol.h"

#include <cstdio>
#include <cstring>

namespace {

void writeLe16(uint8_t* dest, uint16_t value) {
  dest[0] = static_cast<uint8_t>(value & 0xFF);
  dest[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint16_t readLe16(const uint8_t* src) {
  return static_cast<uint16_t>(src[0] | (static_cast<uint16_t>(src[1]) << 8));
}

bool appendEscaped(char*& dest, char* end, char ch) {
  if (ch == '"' || ch == '\\') {
    if (dest + 2 > end) {
      return false;
    }
    *dest++ = '\\';
    *dest++ = ch;
    return true;
  }

  if (ch == '\n') {
    if (dest + 2 > end) {
      return false;
    }
    *dest++ = '\\';
    *dest++ = 'n';
    return true;
  }

  if (ch == '\r') {
    if (dest + 2 > end) {
      return false;
    }
    *dest++ = '\\';
    *dest++ = 'r';
    return true;
  }

  if (dest + 1 > end) {
    return false;
  }
  *dest++ = ch;
  return true;
}

}  // namespace

size_t serialPackMessage(
  const SerialHeader& header,
  const uint8_t* payload,
  size_t payloadLen,
  uint8_t* out,
  size_t outMax
) {
  if (out == nullptr || outMax < kSerialHeaderSize + payloadLen) {
    return 0;
  }
  if (payloadLen > 0xFFFF) {
    return 0;
  }
  if (payloadLen > 0 && payload == nullptr) {
    return 0;
  }

  out[0] = static_cast<uint8_t>(header.type);
  out[1] = header.ver;
  writeLe16(out + 2, header.id);
  out[4] = header.flags;
  writeLe16(out + 5, static_cast<uint16_t>(payloadLen));
  if (payloadLen > 0) {
    std::memcpy(out + kSerialHeaderSize, payload, payloadLen);
  }
  return kSerialHeaderSize + payloadLen;
}

bool serialParseHeader(const uint8_t* data, size_t length, SerialHeader& out, size_t& headerBytes) {
  headerBytes = kSerialHeaderSize;
  if (data == nullptr || length < kSerialHeaderSize) {
    return false;
  }

  out.type = static_cast<SerialMsgType>(data[0]);
  out.ver = data[1];
  out.id = readLe16(data + 2);
  out.flags = data[4];
  out.len = readLe16(data + 5);

  if (length < kSerialHeaderSize + out.len) {
    return false;
  }
  return true;
}

size_t serialFormatSyncJson(bool provisioning, const char* linkMode, char* out, size_t outMax) {
  if (out == nullptr || outMax == 0) {
    return 0;
  }
  if (linkMode == nullptr) {
    linkMode = "";
  }

  const int written = std::snprintf(
    out,
    outMax,
    "{\"proto\":%u,\"provisioning\":%s,\"link_mode\":\"%s\"}",
    static_cast<unsigned>(kSerialProtoVersion),
    provisioning ? "true" : "false",
    linkMode
  );
  if (written <= 0 || static_cast<size_t>(written) >= outMax) {
    return 0;
  }
  return static_cast<size_t>(written);
}

size_t serialFormatLogJson(const char* text, char* out, size_t outMax) {
  if (out == nullptr || outMax < 8 || text == nullptr) {
    return 0;
  }

  char* dest = out;
  char* end = out + outMax - 1;
  const char prefix[] = "{\"t\":\"";
  for (const char* p = prefix; *p != '\0'; ++p) {
    if (dest >= end) {
      return 0;
    }
    *dest++ = *p;
  }

  for (const char* p = text; *p != '\0'; ++p) {
    if (!appendEscaped(dest, end, *p)) {
      return 0;
    }
  }

  if (dest + 2 > end) {
    return 0;
  }
  *dest++ = '"';
  *dest++ = '}';
  *dest = '\0';
  return static_cast<size_t>(dest - out);
}
