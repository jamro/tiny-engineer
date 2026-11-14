#pragma once

#include <cstddef>
#include <cstdint>

#include "display/eyes/styles/kawaii/speech.h"

// Scripted lines the mouth says in sync with the stock voice clips.

namespace kawaii {

// One mouth keyframe: at `atMs` into the clip, `openPct` open toward
// `vowel`. Openness ramps between keyframes; a closing keyframe (0) is a lip
// consonant (p, b, m) or a stop (t, d, k).
struct Viseme {
  uint16_t atMs;
  Vowel vowel;
  uint8_t openPct;
};

// The mouth for a keyframe track at elapsedMs; closed outside the track.
Voice mouthLine(const Viseme* track, size_t count, uint32_t elapsedMs);

// The stock clips' lines, keyed to their assets/*.wav loudness envelopes.
enum class Line : uint8_t {
  HelloHuman,    // welcome.wav: "Hello, human. What are we building today?"
  PsstHuman,     // attention.wav: "Psst... human... you might want to take a look."
  UhOhHuman,     // error.wav: "Uh-oh. Human, we have a problem."
  FineAnyway,    // abort.wav: "Fine. I didn't want to finish it anyway."
  ShuttingDown,  // dead.wav: "Insufficient resources... shutting doooowwwnnn"
  Count
};

// The mouth for `line` at elapsedMs from the clip's playback start.
Voice say(Line line, uint32_t elapsedMs);

// True when a clip's cue times (in its .cue field order) match the stock
// clip `line` was keyed to. A custom clip with its own .cue says something
// else.
bool fitsStock(Line line, const uint32_t* cues, size_t count);

}  // namespace kawaii
