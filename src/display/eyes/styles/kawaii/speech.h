#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/expression.h"
#include "display/eyes/styles/kawaii/util.h"

// Talking mouth: syllables open the mouth into vowel shapes. With /play audio
// the opening follows loudness; without it the babbler invents a rhythm.

namespace kawaii {

enum class Vowel : uint8_t {
  Ah,  // tall open "D"
  Ee,  // wide, shallow grin
  Oo,  // small round "o"
  Count
};

struct Voice {
  float level;  // 0 resting .. 1 fully open
  Vowel vowel;
};

// Opens the expression's resting mouth toward the vowel's shape by `level`.
void applyVoice(Expression& target, const Voice& voice);

class Babbler {
 public:
  explicit Babbler(RandRange rand) : rand_(rand) {}

  // Start over with a short pause before the first syllable.
  void reset(uint32_t now);

  // No audio: phrases of syllables with pauses between them.
  Voice babble(uint32_t now);

  // Audio from /play: loudness (0..1) opens the mouth, and each onset
  // (emphasis 0..1) picks a new vowel. The level is damped while the
  // speech gate is closed, so phrase tails close softly.
  Voice follow(uint32_t now, float level, float emphasis, bool speaking);

 private:
  Vowel pickVowel();
  void startPhrase(uint32_t now);
  void startSyllable(uint32_t now);
  void rest(uint32_t now);

  RandRange rand_;
  bool armed_ = false;
  bool inPhrase_ = false;  // false: resting until syllableAt_
  Vowel vowel_ = Vowel::Ah;
  uint32_t syllableAt_ = 0;
  uint32_t syllableMs_ = 0;
  uint32_t syllablesLeft_ = 0;
  float peak_ = 0.0f;
  uint32_t nextVowelAt_ = 0;
};

}  // namespace kawaii
