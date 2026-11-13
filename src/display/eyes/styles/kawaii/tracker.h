#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

struct Gaze {
  float x;  // -1 left .. 1 right
  float y;  // -1 up .. 1 down
};

// Typing: the eyes read code. They follow a few lines word by word, sweeping
// back at each line end, then dart to another spot in the file and read a
// few lines there. The animator eases the result, so steps become glides.
class CodeReader {
 public:
  explicit CodeReader(RandRange rand) : rand_(rand) {}

  // Start reading at a fresh spot; call when typing begins.
  void reset(uint32_t now);
  // Gaze at `now`. Safe across the millis() wrap.
  Gaze update(uint32_t now);

 private:
  void dart(uint32_t now);
  void nextWord(uint32_t now);
  void nextLine(uint32_t now);
  uint32_t between(uint32_t lo, uint32_t hi);  // rand in [lo, hi]

  RandRange rand_;
  bool armed_ = false;
  float lineStart_ = 0.0f;
  float x_ = 0.0f;
  float y_ = 0.0f;
  uint32_t linesLeft_ = 0;
  uint32_t nextAt_ = 0;
};

}  // namespace kawaii
