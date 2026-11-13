#pragma once

#include <cstddef>
#include <cstdint>

// Kawaii face renderer: big round eyes with sparkles, a little mouth, and
// dithered blush. Every feature is a continuous parameter so the animator
// can ease between moods. Pure C++ (no Arduino), so it builds for native
// tests and host previews.

namespace kawaii {

constexpr int16_t kWidth = 128;
constexpr int16_t kHeight = 32;
// Row-major, MSB = leftmost pixel: the layout Adafruit_GFX::drawBitmap reads.
constexpr size_t kFrameBytes = (kWidth / 8) * kHeight;

enum class EyeKind : uint8_t {
  Round,    // filled eye with sparkle highlights
  Chevron,  // > <
  Heart,
  Cross     // x x
};

struct EyeShape {
  EyeKind kind;
  int8_t side;    // +1 screen-left eye (inner corner at +x), -1 screen-right
  float cx;       // center, px
  float cy;
  float rx;       // radii when fully open, px
  float ry;
  float open;     // 0 closed .. 1 open (blinks, sleep, kind swaps)
  float happy;    // 0 round .. 1 "∩" crescent (cheeks push the lower lid up)
  float lid;      // 0 none .. 1 upper lid lowered past the center
  float tilt;     // + lowers the inner corner (angry), - the outer (sad)
  float sparkle;  // highlight size; 0 hides them
};

struct Mouth {
  float cx;         // center of the lip line, px
  float cy;
  float width;      // px
  float curve;      // sag at the center, px: + smile, - frown
  float omega;      // 0 single curve .. 1 cat "ω"
  float openUp;     // px the mouth opens above the lip line
  float openDown;   // px the mouth opens below the lip line
  float wave;       // wobble amplitude, px
  bool hollow;      // outline only ("o") instead of filled
};

struct Face {
  EyeShape left;
  EyeShape right;
  Mouth mouth;
  float blush;  // 0..1 dither density under the outer eye corners
  // Twinkle stars beside the eyes: hidden below 0.3, then a plus sign whose
  // arms are (glint - 0.3) px, rounded down. 3 gives the largest (2 px arms).
  float glint;
};

// Draws `face` into `frame` (cleared first). False if the buffer is too small.
// Pure: no state is kept between calls.
bool render(const Face& face, uint8_t* frame, size_t frameBytes);

// True if pixel (x, y) is lit in a frame produced by render().
bool pixelAt(const uint8_t* frame, int x, int y);

}  // namespace kawaii
