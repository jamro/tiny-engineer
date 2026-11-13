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

// The bell, as one continuous face:
// - wind-up: eager, then suspense builds (eyes narrow onto the bell, the
//   mouth holds its breath, a nervous tremble grows) and holds until...
// - the ding: eyes pop wide, stars, a hop, an open grin;
// - bliss while the bell rings: open sparkling eyes, a delighted shiver;
// - when the ring ends, the shiver winds down, smaller and slower, like the
//   bell dying away, into a dreamy afterglow.
// Keyed to the bell clip; without one, the ding and its end come on time.
class BellCelebration {
 public:
  // Call when the ring starts.
  void reset(uint32_t now);
  // The face at `now`; `bellPlaying`: the bell clip is playing.
  Expression update(uint32_t now, bool bellPlaying);

 private:
  Expression windUp(uint32_t now) const;
  Expression afterDing(uint32_t now) const;

  uint32_t startedMs_ = 0;
  bool rung_ = false;
  bool heard_ = false;   // the clip actually played
  bool ended_ = false;
  uint32_t dingMs_ = 0;
  uint32_t endMs_ = 0;
};

}  // namespace kawaii
