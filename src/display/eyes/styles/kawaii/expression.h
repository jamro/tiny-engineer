#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/face.h"

// Moods and the expression each one targets. Pure data: easing lives in
// animator.h, the personalities (base moods and accents) in director.h,
// speech in speech.h.

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
  // Accents while working (see director.h).
  Pleased,    // a content smile mid-task: work is going well
  Aha,        // thinking: an idea lands, eyes up and bright
  // The bell (ring): see BellCelebration in director.h.
  Suspense,   // a full deep breath before the ding: risen, wide, inhaling "o"
  Yell,       // the ding: eyes pop, a huge excited yell
  Bliss,      // joy while the bell rings: open sparkling eyes
  Afterglow,  // dreamy half-closed happy eyes, ω
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
  float rise;           // px the whole face lifts (a deep breath)
  float quiver;         // pupil shake, px each way (not eased: set per frame)
  float quiverHz;       // how fast the pupils shake (the phase carries on)
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

// a..b by t (0..1) on every eased value; the eye kind switches at the end.
Expression blend(const Expression& a, const Expression& b, float t);

}  // namespace kawaii
