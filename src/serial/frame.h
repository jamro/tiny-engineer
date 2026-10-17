#pragma once

#include <cstddef>
#include <cstdint>

// Max decoded application payload inside one COBS frame (JSON or stream chunk).
constexpr size_t kSerialMaxPayload = 2048;

// Worst-case encoded size including trailing COBS delimiter.
size_t serialFrameEncodedMax(size_t payloadLen);

// Encode |payload| into |out| (must hold serialFrameEncodedMax(payloadLen)).
// Writes COBS frame ending with 0x00. Returns encoded length, or 0 on failure.
size_t serialFrameEncode(const uint8_t* payload, size_t payloadLen, uint8_t* out, size_t outMax);

// Incremental COBS frame decoder with resync on the next 0x00 after errors.
class SerialFrameDecoder {
public:
  using FrameHandler = void (*)(const uint8_t* payload, size_t length, void* user);

  void reset();
  void setHandler(FrameHandler handler, void* user);

  // Feed raw CDC bytes. Invokes handler for each successfully decoded frame.
  void feed(const uint8_t* data, size_t length);

  // True after a decode failure until the next successful frame (or reset).
  bool resyncing() const;

private:
  static constexpr size_t kEncodedMax = 1 + kSerialMaxPayload + ((kSerialMaxPayload + 253) / 254) + 1;

  void onDelimiter_();

  uint8_t buf_[kEncodedMax] = {};
  size_t len_ = 0;
  bool overflow_ = false;
  bool resyncing_ = false;
  FrameHandler handler_ = nullptr;
  void* user_ = nullptr;
};
