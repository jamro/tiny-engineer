#pragma once

#include <cstddef>
#include <cstdint>

// Speech-energy features for talking animation. No servos/eyes/HTTP.
// Fed from /play PCM when the play pose is talking; ignored otherwise.

struct AudioMotionSnapshot {
  bool streamActive;
  bool speaking;
  float level;     // 0..1 smoothed loudness
  float emphasis;  // short-lived onset strength 0..1
  uint32_t phraseId;
};

void audioMotionBegin(uint32_t nowMs);
void audioMotionFeed(const int16_t* samples, size_t count, uint32_t nowMs);
void audioMotionEnd(uint32_t nowMs);
AudioMotionSnapshot audioMotionSnapshot(uint32_t nowMs);
