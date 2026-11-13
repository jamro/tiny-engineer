#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/face.h"

// Moods and the expression each one targets. Pure data: easing lives in
// animator.h, the idle personality in idle.h, speech in speech.h.

namespace kawaii {

enum class Mood : uint8_t {
  // Idle family: mostly smiles, with the occasional accent.
  Content,    // ω cat mouth
  Smile,      // ‿
  Grin,       // ^ ^ happy crescents, open smile
  WinkLeft,
  WinkRight,
  Excited,    // bigger eyes, hop, twinkles, open grin
  Love,       // heart eyes
  // Mode moods.
  Focused,    // typing
  Reading,
  Thinking,
  Surprised,  // ring
  Curious,    // attention
  Angry,      // error
  Panic,      // abort: > <
  Sleepy,     // wakeup yawn
  Sad,        // dead, before the X
  Dizzy,      // dead: x x
  Talking,    // resting mouth; speech.h opens it per syllable
  Asleep,
  Count
};

// snake_case name, or nullptr for Mood::Count.
const char* moodName(Mood mood);

// True for the idle family (Content .. Love), which is listed first.
constexpr bool isIdleMood(Mood mood) {
  return mood <= Mood::Love;
}

// Every value the animator eases. Geometry (where the eyes are, how tall,
// blinks) comes from the shared pose and is not part of the expression.
struct Expression {
  EyeKind kind;
  float happyLeft;      // 0..1, see EyeShape::happy
  float happyRight;
  float lid;            // 0..1, see EyeShape::lid
  float tilt;           // see EyeShape::tilt
  float pupil;          // pupil size multiplier (0 hides it)
  float gazeX;          // mood's own look, added to the pose glance, -1..1
  float gazeY;          // -1 up .. 1 down
  float followGlance;   // 0..1 of the choreography's glances to follow
  float sparkle;        // catch-light size multiplier
  float scale;          // eye radius multiplier
  float blush;          // 0..1
  float glint;          // 0..1 twinkle star strength
  float bounce;         // hop height, px
  float mouthWidth;     // px
  float mouthCurve;     // px, see Mouth::curve
  float mouthOmega;     // 0..1, see Mouth::omega
  float mouthOpenUp;    // px
  float mouthOpenDown;  // px
  float mouthWave;      // px
  float mouthShift;     // px sideways
  float mouthHollow;    // 0 filled .. 1 outline
};

// The preset for `mood`. The numbers in each preset are hand-tuned looks,
// labelled by the mood they belong to.
Expression expressionFor(Mood mood);

}  // namespace kawaii
