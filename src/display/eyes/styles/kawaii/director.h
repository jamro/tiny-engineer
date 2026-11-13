#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/expression.h"
#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

// What the robot is busy with; each has its own personality.
enum class Activity : uint8_t {
  Idle,      // Content / Smile; grin, wink, excited, or heart-eye accents
  Typing,    // Focused; pleased smile, grin, or wink accents
  Reading,   // Reading; pleased, "ooh" curious, or grin accents
  Thinking,  // Thinking; "aha!" or pleased accents
};

// Plays an activity's base mood and, every so often, a short accent before
// settling back. `flow` (0..1, how long work has gone well) makes accents
// come more often while working.
class MoodDirector {
 public:
  explicit MoodDirector(RandRange rand) : rand_(rand) {}

  // Start an activity from its base mood.
  void reset(uint32_t now, Activity activity);
  // The mood to show at `now`. Safe across the millis() wrap.
  Mood update(uint32_t now, float flow = 0.0f);

  Activity activity() const { return activity_; }

 private:
  Mood pickAccent(uint32_t now);
  void endAccent(uint32_t now, float flow);

  RandRange rand_;
  Activity activity_ = Activity::Idle;
  bool armed_ = false;
  Mood base_ = Mood::Smile;
  bool accentActive_ = false;
  Mood accent_ = Mood::Grin;
  uint32_t baseSwapAt_ = 0;
  uint32_t accentAt_ = 0;
  uint32_t accentEndAt_ = 0;
};

// The bell: an eager wind-up, a burst of joy on the ding, bliss while the
// bell sustains, then a dreamy afterglow. Keyed to the moment the bell
// clip starts; if it never plays (no clip on the filesystem), the ding
// comes after a wait instead.
class BellCelebration {
 public:
  // Call when the ring starts.
  void reset(uint32_t now);
  // `bellPlaying`: the bell clip is playing right now.
  Mood update(uint32_t now, bool bellPlaying);

 private:
  uint32_t startedMs_ = 0;
  bool rung_ = false;
  uint32_t dingMs_ = 0;
};

}  // namespace kawaii
