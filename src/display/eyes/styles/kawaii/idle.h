#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/expression.h"
#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

// Idle personality: alternates Content and Smile, and every few seconds
// plays a short accent (grin, wink, excited, or rarely heart eyes).
class IdleDirector {
 public:
  explicit IdleDirector(RandRange rand) : rand_(rand) {}

  // Start over from a smile; call when idle begins.
  void reset(uint32_t now);
  // The mood to show at `now`. Safe across the millis() wrap.
  Mood update(uint32_t now);

 private:
  Mood pickAccent(uint32_t now);
  void endAccent(uint32_t now);

  RandRange rand_;
  bool armed_ = false;
  Mood base_ = Mood::Smile;
  bool accentActive_ = false;
  Mood accent_ = Mood::Grin;
  uint32_t baseSwapAt_ = 0;
  uint32_t accentAt_ = 0;
  uint32_t accentEndAt_ = 0;
};

}  // namespace kawaii
