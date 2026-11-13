#include "display/eyes/styles/kawaii/director.h"

#include <cstddef>

namespace kawaii {

namespace {

struct RangeMs {
  uint32_t min;
  uint32_t max;
};

// One accent: its odds (percent; an activity's odds sum to 100) and how long
// it plays. WinkLeft stands for either wink; the side is a coin flip.
struct Accent {
  Mood mood;
  uint32_t chancePct;
  RangeMs duration;
};

constexpr size_t kMaxAccents = 4;

struct Personality {
  Mood base;          // shown between accents, and settled on after one
  Mood alternate;     // swapped with `base` every baseHold (same = no swap)
  RangeMs baseHold;
  RangeMs settleHold;  // base after an accent, before swapping again
  RangeMs firstAccent;  // delay after the activity starts
  RangeMs accentGap;    // between accents, at flow 0
  RangeMs flowGap;      // between accents, at full flow
  Accent accents[kMaxAccents];
};

constexpr uint32_t kPercent = 100;

// Indexed by Activity.
constexpr Personality kPersonalities[] = {
  // Idle: mostly smiles, with the occasional accent.
  {Mood::Smile, Mood::Content, {3000, 8000}, {2500, 6000}, {3500, 8000},
   {4500, 10000}, {4500, 10000},
   {{Mood::Grin, 34, {1400, 2600}},
    {Mood::WinkLeft, 30, {550, 900}},
    {Mood::Excited, 26, {1200, 1900}},
    {Mood::Love, 10, {1500, 2200}}}},
  // Typing: focused, with a pleased smile now and then; more often while
  // work keeps going well.
  {Mood::Focused, Mood::Focused, {0, 0}, {0, 0}, {5000, 10000},
   {14000, 24000}, {6000, 11000},
   {{Mood::Pleased, 55, {1500, 2800}},
    {Mood::Grin, 25, {800, 1400}},
    {Mood::WinkLeft, 20, {500, 800}}}},
  // Reading: absorbed, with happy reading and the odd "ooh, interesting".
  {Mood::Reading, Mood::Reading, {0, 0}, {0, 0}, {6000, 12000},
   {12000, 22000}, {8000, 14000},
   {{Mood::Pleased, 65, {1800, 3000}},
    {Mood::Curious, 25, {900, 1500}},
    {Mood::Grin, 10, {800, 1300}}}},
  // Thinking: pondering, with an "aha!" now and then.
  {Mood::Thinking, Mood::Thinking, {0, 0}, {0, 0}, {6000, 12000},
   {9000, 16000}, {7000, 12000},
   {{Mood::Aha, 85, {1100, 1700}},
    {Mood::Pleased, 15, {1200, 2000}}}},
};

constexpr uint32_t totalChance(const Personality& p) {
  uint32_t sum = 0;
  for (const Accent& a : p.accents) {
    sum += a.chancePct;
  }
  return sum;
}
static_assert(totalChance(kPersonalities[0]) == kPercent, "idle odds must sum to 100");
static_assert(totalChance(kPersonalities[1]) == kPercent, "typing odds must sum to 100");
static_assert(totalChance(kPersonalities[2]) == kPercent, "reading odds must sum to 100");
static_assert(totalChance(kPersonalities[3]) == kPercent, "thinking odds must sum to 100");

const Personality& personality(Activity activity) {
  return kPersonalities[(size_t)activity];
}

const Accent& accentForRoll(const Personality& p, uint32_t roll) {
  for (const Accent& a : p.accents) {
    if (roll < a.chancePct) {
      return a;
    }
    roll -= a.chancePct;
  }
  return p.accents[0];
}

uint32_t lerpMs(uint32_t a, uint32_t b, float t) {
  return (uint32_t)((float)a + ((float)b - (float)a) * clamp01(t));
}

}  // namespace

void MoodDirector::reset(uint32_t now, Activity activity) {
  const Personality& p = personality(activity);
  activity_ = activity;
  armed_ = true;
  base_ = p.base;
  accentActive_ = false;
  baseSwapAt_ = now + rand_(p.baseHold.min, p.baseHold.max);
  accentAt_ = now + rand_(p.firstAccent.min, p.firstAccent.max);
}

Mood MoodDirector::pickAccent(uint32_t now) {
  const Accent& accent = accentForRoll(personality(activity_), rand_(0, kPercent));
  accent_ = accent.mood;
  if (accent_ == Mood::WinkLeft && rand_(0, 2) != 0) {
    accent_ = Mood::WinkRight;
  }

  accentActive_ = true;
  accentEndAt_ = now + rand_(accent.duration.min, accent.duration.max);
  return accent_;
}

void MoodDirector::endAccent(uint32_t now, float flow) {
  const Personality& p = personality(activity_);
  accentActive_ = false;
  accentAt_ = now + rand_(
    lerpMs(p.accentGap.min, p.flowGap.min, flow),
    lerpMs(p.accentGap.max, p.flowGap.max, flow)
  );
  base_ = p.base;
  baseSwapAt_ = now + rand_(p.settleHold.min, p.settleHold.max);
}

Mood MoodDirector::update(uint32_t now, float flow) {
  if (!armed_) {
    reset(now, activity_);
  }

  if (accentActive_) {
    if (!reached(now, accentEndAt_)) {
      return accent_;
    }
    endAccent(now, flow);
  }

  if (reached(now, accentAt_)) {
    return pickAccent(now);
  }

  const Personality& p = personality(activity_);
  if (p.alternate != p.base && reached(now, baseSwapAt_)) {
    base_ = base_ == p.base ? p.alternate : p.base;
    baseSwapAt_ = now + rand_(p.baseHold.min, p.baseHold.max);
  }

  return base_;
}

namespace {

constexpr uint32_t kBellWaitMs = 1500;  // no bell by then: ding anyway
constexpr uint32_t kDingMs = 350;       // the burst of joy
constexpr uint32_t kBlissMs = 1600;     // bliss while the bell sustains

}  // namespace

void BellCelebration::reset(uint32_t now) {
  startedMs_ = now;
  rung_ = false;
}

Mood BellCelebration::update(uint32_t now, bool bellPlaying) {
  if (!rung_ && (bellPlaying || reached(now, startedMs_ + kBellWaitMs))) {
    rung_ = true;
    dingMs_ = now;
  }
  if (!rung_) {
    return Mood::Curious;  // winding up
  }

  const uint32_t t = now - dingMs_;
  if (t < kDingMs) {
    return Mood::Excited;
  }
  return t < kBlissMs ? Mood::Bliss : Mood::Afterglow;
}

}  // namespace kawaii
