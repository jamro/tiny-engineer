#include "display/eyes/styles/kawaii/speech.h"

#include <cmath>
#include <cstddef>

namespace kawaii {

namespace {

// Mouth at full loudness for each vowel. Quieter syllables blend from the
// resting mouth toward it.
struct VowelShape {
  float width;     // px
  float curve;     // px
  float openUp;    // px
  float openDown;  // px
  bool hollow;
  uint32_t chancePct;  // how often the babbler picks it
};
constexpr VowelShape kVowels[] = {
  {11.0f, 1.0f, 1.5f, 5.0f, false, 45},  // Ah
  {14.0f, 2.4f, 0.4f, 2.4f, false, 30},  // Ee
  {7.0f, 0.0f, 2.6f, 3.0f, true, 25},    // Oo
};
static_assert(
  sizeof(kVowels) / sizeof(kVowels[0]) == (size_t)Vowel::Count,
  "every vowel needs a shape"
);
constexpr uint32_t kPercent = 100;

// An "o" only reads as a ring once it is open this far.
constexpr float kHollowFromLevel = 0.35f;

// Babble rhythm.
constexpr uint32_t kSyllableMinMs = 110;
constexpr uint32_t kSyllableMaxMs = 220;
constexpr uint32_t kSyllablesMin = 3;
constexpr uint32_t kSyllablesMax = 10;
constexpr uint32_t kPauseMinMs = 250;  // between phrases
constexpr uint32_t kPauseMaxMs = 750;
constexpr uint32_t kLeadInMinMs = 120;  // before the first phrase
constexpr uint32_t kLeadInMaxMs = 300;
constexpr uint32_t kPeakMinPct = 40;  // syllable loudness
constexpr uint32_t kPeakMaxPct = 100;

// Following audio.
constexpr float kOnsetEmphasis = 0.25f;
constexpr uint32_t kMinVowelHoldMs = kSyllableMinMs;
constexpr float kGateClosedLevelScale = 0.3f;

}  // namespace

void applyVoice(Expression& target, const Voice& voice) {
  const size_t index = (size_t)voice.vowel;
  if (index >= (size_t)Vowel::Count) {
    return;
  }

  const VowelShape& open = kVowels[index];
  const float level = clamp01(voice.level);
  target.mouthWidth = lerp(target.mouthWidth, open.width, level);
  target.mouthCurve = lerp(target.mouthCurve, open.curve, level);
  target.mouthOpenUp = lerp(target.mouthOpenUp, open.openUp, level);
  target.mouthOpenDown = lerp(target.mouthOpenDown, open.openDown, level);
  target.mouthHollow = open.hollow && level >= kHollowFromLevel ? 1.0f : 0.0f;
}

void speakingMouth(Expression& target) {
  const Expression talk = expressionFor(Mood::Talking);
  target.mouthWidth = talk.mouthWidth;
  target.mouthCurve = talk.mouthCurve;
  target.mouthOmega = talk.mouthOmega;
  target.mouthOpenUp = talk.mouthOpenUp;
  target.mouthOpenDown = talk.mouthOpenDown;
  target.mouthWave = talk.mouthWave;
  target.mouthShift = talk.mouthShift;
  target.mouthHollow = talk.mouthHollow;
}

void Babbler::reset(uint32_t now) {
  armed_ = true;
  inPhrase_ = false;
  syllableAt_ = now + rand_(kLeadInMinMs, kLeadInMaxMs + 1);
  nextVowelAt_ = now;
}

Vowel Babbler::pickVowel() {
  uint32_t roll = rand_(0, kPercent);
  for (size_t i = 0; i < (size_t)Vowel::Count; i++) {
    if (roll < kVowels[i].chancePct) {
      return (Vowel)i;
    }
    roll -= kVowels[i].chancePct;
  }
  return Vowel::Ah;
}

void Babbler::startPhrase(uint32_t now) {
  inPhrase_ = true;
  syllablesLeft_ = rand_(kSyllablesMin, kSyllablesMax + 1);
  startSyllable(now);
}

void Babbler::startSyllable(uint32_t now) {
  syllablesLeft_--;
  syllableAt_ = now;
  syllableMs_ = rand_(kSyllableMinMs, kSyllableMaxMs + 1);
  peak_ = (float)rand_(kPeakMinPct, kPeakMaxPct + 1) / (float)kPercent;
  vowel_ = pickVowel();
}

void Babbler::rest(uint32_t now) {
  inPhrase_ = false;
  syllableAt_ = now + rand_(kPauseMinMs, kPauseMaxMs + 1);
}

Voice Babbler::babble(uint32_t now) {
  if (!armed_) {
    reset(now);
  }
  if (!reached(now, syllableAt_)) {
    return {0.0f, vowel_};  // resting between phrases
  }

  if (!inPhrase_) {
    startPhrase(now);
  } else if (now - syllableAt_ >= syllableMs_) {
    if (syllablesLeft_ == 0) {
      rest(now);
      return {0.0f, vowel_};
    }
    startSyllable(now);
  }

  // Each syllable opens and closes once.
  const float t = (float)(now - syllableAt_) / (float)syllableMs_;
  return {peak_ * sinf(kPi * t), vowel_};
}

Voice Babbler::follow(uint32_t now, float level, float emphasis, bool speaking) {
  if (!armed_) {
    reset(now);
  }

  if (speaking && emphasis >= kOnsetEmphasis && reached(now, nextVowelAt_)) {
    vowel_ = pickVowel();
    nextVowelAt_ = now + kMinVowelHoldMs;
  }
  return {speaking ? level : level * kGateClosedLevelScale, vowel_};
}

}  // namespace kawaii
