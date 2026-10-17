#include "serial/frame.h"

#include <cobs.h>

size_t serialFrameEncodedMax(size_t payloadLen) {
  return COBS_ENCODE_MAX(payloadLen);
}

size_t serialFrameEncode(const uint8_t* payload, size_t payloadLen, uint8_t* out, size_t outMax) {
  if (payloadLen > kSerialMaxPayload || out == nullptr) {
    return 0;
  }

  // nanocobs rejects a null decoded pointer even when length is 0.
  const uint8_t empty = 0;
  const uint8_t* src = payloadLen == 0 ? &empty : payload;
  if (src == nullptr) {
    return 0;
  }

  size_t encodedLen = 0;
  if (cobs_encode(src, payloadLen, out, outMax, &encodedLen) != COBS_RET_SUCCESS) {
    return 0;
  }

  return encodedLen;
}

void SerialFrameDecoder::reset() {
  len_ = 0;
  overflow_ = false;
  resyncing_ = false;
}

void SerialFrameDecoder::setHandler(FrameHandler handler, void* user) {
  handler_ = handler;
  user_ = user;
}

bool SerialFrameDecoder::resyncing() const {
  return resyncing_;
}

void SerialFrameDecoder::feed(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    return;
  }

  for (size_t i = 0; i < length; ++i) {
    const uint8_t byte = data[i];
    if (byte == COBS_FRAME_DELIMITER) {
      onDelimiter_();
      continue;
    }

    if (overflow_) {
      continue;
    }

    if (len_ >= sizeof(buf_)) {
      overflow_ = true;
      resyncing_ = true;
      continue;
    }

    buf_[len_++] = byte;
  }
}

void SerialFrameDecoder::onDelimiter_() {
  // Include the delimiter for cobs_decode (expects trailing 0x00).
  if (overflow_ || len_ == 0) {
    len_ = 0;
    overflow_ = false;
    // Empty delimiter or overflow: stay/enter resync until a good frame arrives.
    if (overflow_) {
      resyncing_ = true;
    }
    return;
  }

  if (len_ + 1 > sizeof(buf_)) {
    len_ = 0;
    overflow_ = false;
    resyncing_ = true;
    return;
  }

  buf_[len_] = COBS_FRAME_DELIMITER;
  const size_t encodedLen = len_ + 1;

  // Static: handler may send frames while this buffer is still live on the call stack.
  static uint8_t decoded[kSerialMaxPayload];
  size_t decodedLen = 0;
  size_t consumed = 0;
  const cobs_ret_t result =
    cobs_decode(buf_, encodedLen, decoded, sizeof(decoded), &decodedLen, &consumed);

  len_ = 0;
  overflow_ = false;

  if (result != COBS_RET_SUCCESS || consumed != encodedLen || decodedLen > kSerialMaxPayload) {
    resyncing_ = true;
    return;
  }

  resyncing_ = false;
  if (handler_ != nullptr) {
    handler_(decoded, decodedLen, user_);
  }
}
