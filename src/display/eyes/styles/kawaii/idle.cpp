#include "display/eyes/styles/kawaii/idle.h"

#include <cstddef>

namespace kawaii {

namespace {

struct RangeMs {
  uint32_t min;
  uint32_t max;
};

// How long Content / Smile holds before swapping to the other.
constexpr RangeMs kBaseHold = {3000, 8000};
// Delay before the first accent after idle starts.
constexpr RangeMs kFirstAccentDelay = {3500, 8000};
// Delay between the end of one accent and the next.
constexpr RangeMs kAccentGap = {4500, 10000};
// After an accent the base settles on Smile for this long.
constexpr RangeMs kSettleHold = {2500, 6000};

// Accent odds (percent, summing to 100) and how long each plays.
struct Accent {
  Mood mood;  // WinkLeft stands for either wink; the side is a coin flip
  uint32_t chancePct;
  RangeMs duration;
};
constexpr Accent kAccents[] = {
  {Mood::Grin, 34, {1400, 2600}},
  {Mood::WinkLeft, 30, {550, 900}},
  {Mood::Excited, 26, {1200, 1900}},
  {Mood::Love, 10, {1500, 2200}},
};
constexpr uint32_t kPercent = 100;

constexpr uint32_t totalChance() {
  uint32_t sum = 0;
  for (const Accent& a : kAccents) {
    sum += a.chancePct;
  }
  return sum;
}
static_assert(totalChance() == kPercent, "accent odds must sum to 100");

const Accent& accentForRoll(uint32_t roll) {
  for (const Accent& a : kAccents) {
    if (roll < a.chancePct) {
      return a;
    }
    roll -= a.chancePct;
  }
  return kAccents[0];
}

}  // namespace

void IdleDirector::reset(uint32_t now) {
  armed_ = true;
  base_ = Mood::Smile;
  accentActive_ = false;
  baseSwapAt_ = now + rand_(kBaseHold.min, kBaseHold.max);
  accentAt_ = now + rand_(kFirstAccentDelay.min, kFirstAccentDelay.max);
}

Mood IdleDirector::pickAccent(uint32_t now) {
  const Accent& accent = accentForRoll(rand_(0, kPercent));
  accent_ = accent.mood;
  if (accent_ == Mood::WinkLeft && rand_(0, 2) != 0) {
    accent_ = Mood::WinkRight;
  }

  accentActive_ = true;
  accentEndAt_ = now + rand_(accent.duration.min, accent.duration.max);
  return accent_;
}

void IdleDirector::endAccent(uint32_t now) {
  accentActive_ = false;
  accentAt_ = now + rand_(kAccentGap.min, kAccentGap.max);
  base_ = Mood::Smile;
  baseSwapAt_ = now + rand_(kSettleHold.min, kSettleHold.max);
}

Mood IdleDirector::update(uint32_t now) {
  if (!armed_) {
    reset(now);
  }

  if (accentActive_) {
    if (!reached(now, accentEndAt_)) {
      return accent_;
    }
    endAccent(now);
  }

  if (reached(now, accentAt_)) {
    return pickAccent(now);
  }

  if (reached(now, baseSwapAt_)) {
    base_ = base_ == Mood::Content ? Mood::Smile : Mood::Content;
    baseSwapAt_ = now + rand_(kBaseHold.min, kBaseHold.max);
  }

  return base_;
}

}  // namespace kawaii
