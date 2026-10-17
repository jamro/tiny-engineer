#pragma once

#include <cstddef>
#include <cstdint>

// Stream payload after SerialHeader: [stream_id:u8][kind:u8][seq:u16 LE][data...]
constexpr uint8_t kSerialStreamKindData = 1;
constexpr uint8_t kSerialStreamKindCredit = 2;
constexpr uint8_t kSerialStreamKindEnd = 3;
constexpr uint8_t kSerialStreamKindError = 4;
constexpr size_t kSerialStreamHdrSize = 4;

constexpr size_t kSerialPlayCreditChunk = 1024;
constexpr size_t kSerialPlayCreditWindows = 3;

// Start a play session for |animName|. Returns false and sets |error| on failure.
// On success fills stream id and initial credit budget.
bool serialPlayBegin(const char* animName, uint8_t& streamId, uint16_t& credits, const char*& error);

// Feed WAV bytes for an active stream. Returns false if rejected (over credits / bad id /
// parse failure) and sets |error| to a short reason (never null on failure).
bool serialPlayFeed(
  uint8_t streamId,
  const uint8_t* data,
  size_t length,
  uint16_t& creditsOut,
  const char*& error
);

// Finish the stream and release busy. |playedMs| set on success.
bool serialPlayEnd(uint8_t streamId, uint32_t& playedMs, const char*& error);

bool serialPlayActive();
