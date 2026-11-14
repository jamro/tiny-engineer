#include "display/eyes/styles/kawaii/director.h"

#include <cmath>
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
constexpr uint32_t kBreathMs = 900;     // eager to a full breath
constexpr uint32_t kYellMs = 300;       // the yell before it eases into bliss
constexpr uint32_t kYellEaseMs = 300;
constexpr uint32_t kAfterglowMs = 500;  // bliss to afterglow, once the ring ends

// Pupil shake from the ring's loudness: a power below 1 keeps the fading
// tail visible to the end; it slows as the ring dies.
constexpr float kShakeMaxPx = 2.0f;
constexpr float kShakeCurve = 0.4f;
constexpr float kShakeSlowHz = 6.0f;
constexpr float kShakeFastHz = 18.0f;

// The stock assets/bell.wav loudness (percent of its peak) from the start
// of the clip: the strike at 10-40 ms, a shimmer, then a long decay that is
// gone by 1400 ms.
struct Level {
  uint16_t atMs;
  uint8_t pct;
};
constexpr Level kBellRing[] = {
  {0, 0}, {15, 85}, {40, 100}, {130, 72}, {160, 30}, {250, 38}, {400, 27},
  {450, 15}, {550, 21}, {700, 17}, {900, 13}, {1000, 10}, {1100, 7},
  {1200, 5}, {1300, 2}, {1400, 0},
};
constexpr size_t kBellRingCount = sizeof(kBellRing) / sizeof(kBellRing[0]);
constexpr uint32_t kRingEndMs = kBellRing[kBellRingCount - 1].atMs;

// Ease in and out, 0..1.
float smooth(float t) {
  t = clamp01(t);
  return t * t * (3.0f - 2.0f * t);
}

float since(uint32_t now, uint32_t at) {
  return (float)(now - at);
}

}  // namespace

void BellCelebration::reset(uint32_t now) {
  startedMs_ = now;
  rung_ = false;
}

float BellCelebration::ring(uint32_t now) const {
  const uint32_t t = sinceDing(now);
  if (!rung_ || t >= kRingEndMs) {
    return 0.0f;
  }
  size_t i = 0;
  while (i + 2 < kBellRingCount && t >= kBellRing[i + 1].atMs) {
    i++;
  }
  const Level& a = kBellRing[i];
  const Level& b = kBellRing[i + 1];
  const float f = (float)(t - a.atMs) / (float)(b.atMs - a.atMs);
  return lerp(a.pct, b.pct, f) / 100.0f;
}

Expression BellCelebration::breath(uint32_t now) const {
  // Breathing in deeper and deeper; eases in so the last of it is the fullest.
  const float t = since(now, startedMs_) / (float)kBreathMs;
  return blend(expressionFor(Mood::Curious), expressionFor(Mood::Suspense), smooth(t));
}

Expression BellCelebration::afterDing(uint32_t now) const {
  const float t = since(now, dingMs_);
  Expression e = expressionFor(Mood::Yell);
  if (t >= (float)kYellMs) {
    e = blend(e, expressionFor(Mood::Bliss), (t - kYellMs) / (float)kYellEaseMs);
  }
  if (t >= (float)kRingEndMs) {
    e = blend(e, expressionFor(Mood::Afterglow), smooth((t - kRingEndMs) / (float)kAfterglowMs));
  }

  // The pupils shake with the bell: hardest on the strike, fading and
  // slowing with the ring, still at its last.
  const float level = ring(now);
  e.quiver = level > 0.0f ? kShakeMaxPx * powf(level, kShakeCurve) : 0.0f;
  e.quiverHz = lerp(kShakeSlowHz, kShakeFastHz, sqrtf(level));
  return e;
}

Expression BellCelebration::update(uint32_t now, bool bellPlaying) {
  if (!rung_ && (bellPlaying || reached(now, startedMs_ + kBellWaitMs))) {
    rung_ = true;
    dingMs_ = now;
  }
  return rung_ ? afterDing(now) : breath(now);
}

}  // namespace kawaii
