#include "display/eyes/styles/kawaii/face.h"

#include <cmath>
#include <cstring>

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

namespace {

constexpr int kRowBytes = kWidth / 8;

// ------------------------------------------------------------- tuning ---
// Sizes are fractions of the eye radii unless they end in Px.

constexpr float kMinEyeRadiusPx = 1.0f;  // smaller eyes are not drawn

// Closed eye: a "‿" lid line, or "∩" when the eye was smiling as it shut.
constexpr float kClosedHalfWidth = 0.9f;
constexpr float kClosedSagPx = 2.2f;
constexpr float kClosedHappyAbove = 0.5f;
// Round eyes flatter than this (height ÷ width radius) draw the lid line: a
// thinner ellipse reads as a bar, not a blink. Compared with the width, since
// scripted blinks squash the pose height itself, not just the open amount.
constexpr float kRoundClosedBelow = 0.22f;
// Line shapes (> <, x, hearts) break into dashes when squashed, so they
// switch to the lid line earlier, at this fraction of their open height.
constexpr float kLineShapeClosedBelow = 0.4f;

// Happy carve: an ellipse rising from below. Wide while subtle so the whole
// lower edge lifts evenly; narrows at full smile to leave a "∩" crescent.
constexpr float kCarveHeight = 0.95f;
constexpr float kCarveWidthSubtle = 1.3f;
constexpr float kCarveWidthNarrowing = 0.35f;
constexpr float kCarveRise = 1.55f;  // at happy = 1 the carve top is this far up

// Upper lid: how far `lid` = 1 lowers it from the top edge.
constexpr float kLidTravel = 1.2f;

// Pupil: a big dark disc that follows the gaze. Its size never changes with
// lids, smiles or blinks: they cover it instead, the way eyelids do, and a
// rim of eye always stays white around it so the outline never notches.
constexpr int kPupilRimPx = 2;
// How far the pupil may tuck under the rim when looking hard to a side, so a
// big pupil still moves visibly; the rim covers the tucked part.
constexpr float kPupilTuckPx = 1.0f;
constexpr float kMinPupilRadiusPx = 1.6f;
// Catch-lights: white spots inside the pupil, the same in both eyes (one big
// round light up and left, two small dots). White on black, so they never
// read as a second pupil. Offsets and radii are × the pupil radius.
struct Light {
  float dx;
  float dy;
  float radius;
};
// Every light keeps at least 1.5 px of dark pupil around it, so the pupil
// still reads as one dark disc.
constexpr Light kCatchLights[] = {
  {-0.3f, -0.3f, 0.32f},
  {0.45f, -0.35f, 0.14f},
  {-0.45f, 0.42f, 0.14f},
};
constexpr float kMinCatchLightRadiusPx = 0.5f;
// Heart eyes have no pupil, just a dark highlight hole: offset from the
// center (× radii) and radius (× the smaller radius).
constexpr float kHeartHoleDx = -0.45f;
constexpr float kHeartHoleDy = -0.42f;
constexpr float kHeartHoleRadius = 0.18f;
constexpr float kHeartHoleMinEyeHeightPx = 5.0f;  // shorter hearts have no room
constexpr float kMinSparkle = 0.05f;
constexpr float kMinHoleRadiusPx = 1.1f;  // smaller reads as a stray pixel, not a sparkle

// Line shapes: extent within the eye box and stroke half-width.
constexpr float kChevronHeight = 0.78f;
constexpr float kChevronWidth = 0.95f;
constexpr float kChevronHalfStrokePx = 1.5f;
constexpr float kCrossHeight = 0.68f;
constexpr float kCrossWidth = 0.62f;
constexpr float kCrossHalfStrokePx = 1.4f;
constexpr float kHeartScale = 0.88f;
constexpr float kHeartScan = 1.05f;  // search box, so the lobes are never clipped
constexpr float kHeartLift = 0.12f;  // centers lobes + tip in the eye box

// Blush: a dithered oval under the outer eye corner.
constexpr float kBlushRxPx = 5.5f;
constexpr float kBlushRyPx = 2.4f;
constexpr float kBlushOutsetPx = 1.5f;  // past the outer eye edge
constexpr float kBlushDrop = 0.6f;       // below the eye center, × rx
constexpr float kBlushEdgeSoftness = 2.2f;
constexpr float kMinBlush = 0.02f;
constexpr float kBayerLevels = 16.0f;
constexpr float kFullBlushCoverage = 0.5f;  // full blush peaks at a checkerboard
constexpr float kBlushDitherScale = kBayerLevels * kFullBlushCoverage;

// Mouth.
constexpr int kMaxMouthColumns = 48;  // wider mouths are skipped
constexpr float kMinMouthWidthPx = 1.0f;
constexpr float kMouthWaveCycles = 3.0f;

// Twinkle stars beside the eyes.
constexpr float kMinGlint = 0.3f;
constexpr float kGlintOutsetPx = 4.5f;  // past the outer eye edge
constexpr float kGlintRise = 0.8f;      // above the eye center, × rx

// 4×4 ordered dither thresholds (0..15).
constexpr uint8_t kBayer[4][4] = {
  {0, 8, 2, 10},
  {12, 4, 14, 6},
  {3, 11, 1, 9},
  {15, 7, 13, 5},
};

// ------------------------------------------------------------ helpers ---

int floorInt(float v) {
  return (int)floorf(v);
}

// Pixel i covers [i, i + 1); shapes are tested at its center.
float center(int i) {
  return i + 0.5f;
}

// Pixels whose centers lie within halfWidth of `mid`.
struct Span {
  int first;
  int last;
};

Span pixelSpan(float mid, float halfWidth) {
  return {(int)ceilf(mid - halfWidth - 0.5f), floorInt(mid + halfWidth - 0.5f)};
}

// Moves `x`, a float-solved estimate, to the last pixel in [lo, hi] where
// `keep` holds, for a test that is true up to a boundary and false after it.
// Returns lo - 1 when none qualify. Checking the edge pixels with the exact
// test keeps the result identical to testing every pixel.
template <typename Keep>
int lastKept(int x, int lo, int hi, Keep keep) {
  x = x < lo - 1 ? lo - 1 : (x > hi ? hi : x);
  while (x >= lo && !keep(x)) {
    x--;
  }
  while (x < hi && keep(x + 1)) {
    x++;
  }
  return x;
}

// Float pixel index clamped to [lo, hi] so the int conversion is defined.
float clampIndex(float v, int lo, int hi) {
  return fminf(fmaxf(v, (float)lo), (float)hi);
}

// The run of pixels in [lo, hi] near mid ± halfWidth for which `inside`
// holds (true on one contiguous run). Empty (first > last) when none do.
template <typename Inside>
Span runAround(float mid, float halfWidth, int lo, int hi, Inside inside) {
  const Span none = {hi + 1, hi};
  const float before = clampIndex(mid - halfWidth - 0.5f, lo - 1, hi + 1);
  const float end = clampIndex(mid + halfWidth - 0.5f, lo - 1, hi + 1);
  const int first = lastKept(floorInt(before), lo, hi, [&](int x) { return !inside(x); }) + 1;
  if (first > hi || !inside(first)) {
    return none;
  }
  return {first, lastKept(floorInt(end), first, hi, inside)};
}

// Pixels of row y within r of (cx, cy); empty (first > last) when none.
Span discRow(float cx, float cy, float r, int y) {
  const float dy = center(y) - cy;
  const float rest = r * r - dy * dy;
  const int lo = floorInt(cx - r);
  const int hi = floorInt(cx + r);
  if (rest < 0.0f) {
    return {hi + 1, hi};
  }
  const float r2 = r * r;
  return runAround(cx, sqrtf(rest), lo, hi, [&](int x) {
    const float dx = center(x) - cx;
    return dx * dx + dy * dy <= r2;
  });
}

class Canvas {
 public:
  explicit Canvas(uint8_t* bits) : bits_(bits) {}

  void plot(int x, int y) { set(x, y, true); }

  // Lights x0..x1 on row y (nothing when x0 > x1).
  void run(int x0, int x1, int y) {
    if (y < 0 || y >= kHeight) {
      return;
    }
    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 >= kWidth ? kWidth - 1 : x1;
    for (int x = x0; x <= x1; x++) {
      set(x, y, true);
    }
  }

  void column(int x, int y0, int y1) {
    if (y0 > y1) {
      const int t = y0;
      y0 = y1;
      y1 = t;
    }
    for (int y = y0; y <= y1; y++) {
      plot(x, y);
    }
  }

  // Lights (on) or clears every pixel whose center is within r of (cx, cy).
  void disc(float cx, float cy, float r, bool on) {
    for (int y = floorInt(cy - r); y <= floorInt(cy + r); y++) {
      const Span span = discRow(cx, cy, r, y);
      for (int x = span.first; x <= span.last; x++) {
        set(x, y, on);
      }
    }
  }

  // Lights (on) or clears the pixels of row y whose bits are set, bit i
  // being pixel x0 + i.
  void bits(int x0, int y, uint64_t mask, bool on) {
    for (int i = 0; mask != 0; i++, mask >>= 1) {
      if (mask & 1u) {
        set(x0 + i, y, on);
      }
    }
  }

  // Plus-shaped twinkle star.
  void star(float cx, float cy, int arm) {
    if (arm < 0) {
      return;
    }

    const int x = floorInt(cx);
    const int y = floorInt(cy);
    plot(x, y);
    for (int i = 1; i <= arm; i++) {
      plot(x - i, y);
      plot(x + i, y);
      plot(x, y - i);
      plot(x, y + i);
    }
  }

 private:
  void set(int x, int y, bool on) {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
      return;
    }

    uint8_t& byte = bits_[y * kRowBytes + x / 8];
    const uint8_t mask = (uint8_t)(0x80u >> (x & 7));
    byte = on ? (uint8_t)(byte | mask) : (uint8_t)(byte & ~mask);
  }

  uint8_t* bits_;
};

// ---------------------------------------------------------------- eyes ---

void drawClosedEye(Canvas& c, const EyeShape& e) {
  const float halfW = e.rx * kClosedHalfWidth;
  if (halfW < 1.0f) {
    return;
  }

  const float sag = e.happy > kClosedHappyAbove ? -kClosedSagPx : kClosedSagPx;
  const Span span = pixelSpan(e.cx, halfW);
  for (int x = span.first; x <= span.last; x++) {
    const float u = (center(x) - e.cx) / halfW;
    // Arc centered on cy; the 2 px line straddles it.
    const float y = e.cy + sag * (1.0f - u * u) - sag * 0.5f - 0.5f;
    const int yi = floorInt(y);
    c.column(x, yi, yi + 1);
  }
}

// The ellipse that carves a happy eye into a crescent.
class HappyCarve {
 public:
  HappyCarve(const EyeShape& e, float ry, float happy)
    : active_(happy > 0.001f),
      rx_(e.rx * (kCarveWidthSubtle - kCarveWidthNarrowing * happy)),
      ry_(ry * kCarveHeight),
      cy_(e.cy + ry - happy * kCarveRise * ry + ry_) {}

  // Pixels of `row` carved out at height py; empty (first > last) if none.
  Span carved(const Span& row, float cx, float py) const {
    const float w = halfWidthAt(py);
    if (w <= 0.0f) {
      return {row.last + 1, row.last};
    }
    return runAround(cx, w, row.first, row.last, [&](int x) { return fabsf(center(x) - cx) < w; });
  }

  // Highest point of the carve; below the eye when there is none.
  float top(float eyeBottom) const { return active_ ? cy_ - ry_ : eyeBottom; }

  // Half-width carved out of row `py`; negative when the row misses it.
  float halfWidthAt(float py) const {
    if (!active_) {
      return -1.0f;
    }
    const float t = (py - cy_) / ry_;
    return t * t < 1.0f ? rx_ * sqrtf(1.0f - t * t) : -1.0f;
  }

 private:
  bool active_;
  float rx_;
  float ry_;
  float cy_;
};

// Straight upper lid, lowered by `lid` and tilted by `tilt`.
class LidCut {
 public:
  LidCut(const EyeShape& e, float ry)
    : cx_(e.cx),
      base_(e.cy - ry + clamp01(e.lid) * kLidTravel * ry),
      slope_(e.tilt * e.side * ry / e.rx) {}

  float edgeAt(float px) const { return base_ + slope_ * (px - cx_); }
  bool hides(float px, float py) const { return py < edgeAt(px); }

  // Narrows `s` to the pixels of row py that are below the lid. False when
  // the lid covers the whole row.
  bool trim(Span& s, float py) const {
    const auto hidden = [&](int x) { return hides(center(x), py); };
    if (slope_ == 0.0f) {
      return !hidden(s.first);
    }

    // Pixel index where the lid line crosses this row.
    const float cross = clampIndex(cx_ + (py - base_) / slope_ - 0.5f, s.first - 1, s.last + 1);
    if (slope_ > 0.0f) {
      // Lid lower toward +x: visible on the left.
      s.last = lastKept(floorInt(cross), s.first, s.last, [&](int x) { return !hidden(x); });
    } else {
      s.first = lastKept((int)ceilf(cross) - 1, s.first, s.last, hidden) + 1;
    }
    return s.first <= s.last;
  }

 private:
  float cx_;
  float base_;
  float slope_;
};

// Gaze inside the unit circle, so diagonal looks travel no further.
void unitGaze(const EyeShape& e, float& gx, float& gy) {
  gx = e.gazeX;
  gy = e.gazeY;
  const float len2 = gx * gx + gy * gy;
  if (len2 > 1.0f) {
    const float len = sqrtf(len2);
    gx /= len;
    gy /= len;
  }
}

// One bit per pixel of a round eye, a row per screen line, so the pupil can
// be clipped to the eye and kept off its edges with cheap bit operations.
class EyeMask {
 public:
  explicit EyeMask(int x0) : x0_(x0) {}

  int x0() const { return x0_; }
  uint64_t row(int y) const { return y >= 0 && y < kHeight ? rows_[y] : 0; }

  // Bits for pixels a..b (clipped to the mask's 64 columns).
  uint64_t span(int a, int b) const {
    a = a < x0_ ? x0_ : a;
    b = b > x0_ + 63 ? x0_ + 63 : b;
    if (a > b) {
      return 0;
    }
    const int n = b - a + 1;
    const uint64_t ones = n >= 64 ? ~(uint64_t)0 : (((uint64_t)1 << n) - 1);
    return ones << (a - x0_);
  }

  void add(int a, int b, int y) {
    if (y >= 0 && y < kHeight) {
      rows_[y] |= span(a, b);
    }
  }

  // Drops every pixel that touches the outside (4-neighbours), `times` over.
  void erode(int times) {
    for (int t = 0; t < times; t++) {
      uint64_t prev = 0;
      for (int y = 0; y < kHeight; y++) {
        const uint64_t cur = rows_[y];
        const uint64_t next = y + 1 < kHeight ? rows_[y + 1] : 0;
        rows_[y] = cur & (cur << 1) & (cur >> 1) & prev & next;
        prev = cur;
      }
    }
  }

 private:
  int x0_;
  uint64_t rows_[kHeight] = {};
};

// A disc centered on a pixel: every pixel (dx, dy) whole pixels from the
// center with dx² + dy² ≤ r². Its shape depends only on r, so each row's
// half-width is worked out once and the disc never changes shape as it moves.
class PixelDisc {
 public:
  explicit PixelDisc(float r) : rows_(r < kMaxRows ? (int)r : kMaxRows) {
    const float r2 = r * r;
    for (int dy = 0; dy <= rows_; dy++) {
      const float rest = r2 - (float)(dy * dy);
      int w = (int)sqrtf(rest);
      // sqrtf may round across a whole number: settle on the exact edge.
      while ((float)((w + 1) * (w + 1)) <= rest) {
        w++;
      }
      while (w >= 0 && (float)(w * w) > rest) {
        w--;
      }
      half_[dy] = w;
    }
  }

  int rows() const { return rows_; }
  // Half-width of row dy (|dy| ≤ rows()); -1 when the row is empty.
  int halfWidth(int dy) const { return half_[dy < 0 ? -dy : dy]; }

 private:
  static constexpr int kMaxRows = 15;
  int rows_;
  int half_[kMaxRows + 1] = {};
};

// Lights (on) or clears the pixels of a disc centered on pixel (cx, cy) that
// are set in `allowed` (a row per screen line, bit i = pixel x0 + i).
// Records what it drew in `drawn` when given.
void maskedDisc(
  Canvas& c, int x0, const uint64_t* allowed, int cx, int cy, const PixelDisc& disc, bool on,
  uint64_t* drawn
) {
  const EyeMask cols(x0);
  for (int dy = -disc.rows(); dy <= disc.rows(); dy++) {
    const int y = cy + dy;
    const int w = disc.halfWidth(dy);
    if (y < 0 || y >= kHeight || w < 0) {
      continue;
    }
    const uint64_t bits = cols.span(cx - w, cx + w) & allowed[y];
    c.bits(x0, y, bits, on);
    if (drawn != nullptr) {
      drawn[y] |= bits;
    }
  }
}

void drawPupil(Canvas& c, const EyeShape& e, EyeMask& eye) {
  const float r = fminf(e.pupil, e.rx - kPupilRimPx - 1.0f);
  if (r < kMinPupilRadiusPx) {
    return;
  }

  float gx = 0.0f;
  float gy = 0.0f;
  unitGaze(e, gx, gy);
  // Travel is the same on both axes, so looking down under a squint or a
  // smile moves the pupil as far as anywhere else; the lid just covers it.
  // Centers snap to whole pixels, so the pupil moves without changing shape.
  const float travel = fmaxf(0.0f, e.rx - r - kPupilRimPx + kPupilTuckPx);
  const int px = floorInt(e.cx + gx * travel);
  const int py = floorInt(e.cy + gy * travel);

  // The pupil may only darken eye pixels at least the rim away from its edge.
  eye.erode(kPupilRimPx);
  uint64_t allowed[kHeight];
  for (int y = 0; y < kHeight; y++) {
    allowed[y] = eye.row(y);
  }
  uint64_t pupil[kHeight] = {};
  maskedDisc(c, eye.x0(), allowed, px, py, PixelDisc(r), false, pupil);

  for (const Light& light : kCatchLights) {
    const float lr = e.sparkle * light.radius * r;
    if (lr >= kMinCatchLightRadiusPx) {
      const int lx = px + (int)roundf(light.dx * r);
      const int ly = py + (int)roundf(light.dy * r);
      maskedDisc(c, eye.x0(), pupil, lx, ly, PixelDisc(lr), true, nullptr);
    }
  }
}

void drawRoundEye(Canvas& c, const EyeShape& e, float ry) {
  const float happy = clamp01(e.happy);
  const HappyCarve carve(e, ry, happy);
  const LidCut lid(e, ry);
  EyeMask mask(floorInt(e.cx - e.rx) - 1);

  // Per row: the ellipse span, trimmed by the lid, minus the carve. Only
  // the edges are tested in float; the runs between them are plain writes.
  for (int y = floorInt(e.cy - ry); y <= floorInt(e.cy + ry); y++) {
    const float py = center(y);
    const float t = (py - e.cy) / ry;
    if (t * t >= 1.0f) {
      continue;
    }

    Span row = pixelSpan(e.cx, e.rx * sqrtf(1.0f - t * t));
    if (row.first > row.last || !lid.trim(row, py)) {
      continue;
    }
    const Span hole = carve.carved(row, e.cx, py);
    c.run(row.first, hole.first - 1, y);
    c.run(hole.last + 1, row.last, y);
    mask.add(row.first, hole.first - 1, y);
    mask.add(hole.last + 1, row.last, y);
  }

  drawPupil(c, e, mask);
}

// Lights the run of row y within halfStroke of x = mid.
void stroke(Canvas& c, float mid, float halfStroke, int y) {
  const int lo = floorInt(mid - halfStroke);
  const Span run = runAround(mid, halfStroke, lo, floorInt(mid + halfStroke), [&](int x) {
    return fabsf(center(x) - mid) <= halfStroke;
  });
  c.run(run.first, run.last, y);
}

// ">" for the screen-left eye, "<" for the right: points at the nose.
void drawChevron(Canvas& c, const EyeShape& e, float ry) {
  const float h = ry * kChevronHeight;
  const float w = e.rx * kChevronWidth;
  const int top = floorInt(e.cy - h);
  const int bottom = floorInt(e.cy + h);

  for (int y = top; y <= bottom; y++) {
    const float a = fabsf(center(y) - e.cy) / h;
    if (a <= 1.0f) {
      stroke(c, e.cx + e.side * (w * 0.5f - w * a), kChevronHalfStrokePx, y);
    }
  }
}

void drawCross(Canvas& c, const EyeShape& e, float ry) {
  const float h = ry * kCrossHeight;
  const float w = e.rx * kCrossWidth;
  const int top = floorInt(e.cy - h);
  const int bottom = floorInt(e.cy + h);

  for (int y = top; y <= bottom; y++) {
    const float d = (center(y) - e.cy) / h;
    if (fabsf(d) <= 1.0f) {
      // The two strokes cross at the center: one at +d·w, the other at -d·w.
      stroke(c, e.cx + d * w, kCrossHalfStrokePx, y);
      stroke(c, e.cx - d * w, kCrossHalfStrokePx, y);
    }
  }
}

constexpr int kMaxHeartCols = 64;  // wider than any eye the panel can show

// Inside the unit heart (X² + Y² − 1)³ − X²Y³ ≤ 0, with X² precomputed.
bool insideHeart(float X2, float Y) {
  const float a = X2 + Y * Y - 1.0f;
  return a * a * a - X2 * Y * Y * Y <= 0.0f;
}

void drawHeart(Canvas& c, const EyeShape& e, float ry) {
  const float sx = e.rx * kHeartScale;
  const float sy = ry * kHeartScale;
  const int top = floorInt(e.cy - ry * kHeartScan);
  const int bottom = floorInt(e.cy + ry * kHeartScan);
  const int left = floorInt(e.cx - e.rx * kHeartScan);
  const int cols = floorInt(e.cx + e.rx * kHeartScan) - left + 1;
  if (cols <= 0 || cols > kMaxHeartCols) {
    return;
  }

  float X2[kMaxHeartCols];
  for (int k = 0; k < cols; k++) {
    const float X = (center(left + k) - e.cx) / sx;
    X2[k] = X * X;
  }

  for (int y = top; y <= bottom; y++) {
    const float Y = (e.cy - center(y)) / sy + kHeartLift;
    for (int k = 0; k < cols; k++) {
      if (insideHeart(X2[k], Y)) {
        c.plot(left + k, y);
      }
    }
  }

  const float hole = e.sparkle * kHeartHoleRadius * fminf(e.rx, ry);
  if (ry >= kHeartHoleMinEyeHeightPx && e.sparkle > kMinSparkle && hole >= kMinHoleRadiusPx) {
    c.disc(e.cx + kHeartHoleDx * e.rx, e.cy + kHeartHoleDy * ry, hole, false);
  }
}

void drawEye(Canvas& c, const EyeShape& e) {
  if (e.rx < kMinEyeRadiusPx) {
    return;
  }

  const float ry = e.ry * clamp01(e.open);
  const float closedBelow =
    e.kind == EyeKind::Round ? kRoundClosedBelow * e.rx : kLineShapeClosedBelow * e.ry;
  if (ry < closedBelow) {
    drawClosedEye(c, e);
    return;
  }

  switch (e.kind) {
    case EyeKind::Round:
      drawRoundEye(c, e, ry);
      break;
    case EyeKind::Chevron:
      drawChevron(c, e, ry);
      break;
    case EyeKind::Heart:
      drawHeart(c, e, ry);
      break;
    case EyeKind::Cross:
      drawCross(c, e, ry);
      break;
  }
}

// --------------------------------------------------------------- blush ---

constexpr int kBlushMaxCols = (int)(2.0f * kBlushRxPx) + 2;
constexpr int kBlushMaxRows = (int)(2.0f * kBlushRyPx) + 2;
static_assert(kBlushMaxCols <= 16, "blush mask rows are 16 bits");

bool nearLit(const uint8_t* frame, int x, int y) {
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (pixelAt(frame, x + dx, y + dy)) {
        return true;
      }
    }
  }
  return false;
}

// d2: squared distance from the blush center in radii (1 = rim).
bool blushDot(int x, int y, float d2, float amount) {
  if (d2 >= 1.0f) {
    return false;
  }
  // Soft edge: density falls off toward the rim.
  const float level = amount * clamp01((1.0f - d2) * kBlushEdgeSoftness);
  return kBayer[y & 3][x & 3] < level * kBlushDitherScale;
}

// Under the outer corner, placed from the open-eye geometry so it holds still
// through blinks. Every dot is decided against the finished eyes before any
// is drawn, so the blush keeps a one-pixel gap from them without counting
// its own dots.
void drawBlush(Canvas& c, const uint8_t* frame, const EyeShape& e, float amount) {
  if (amount <= kMinBlush || e.rx < kMinEyeRadiusPx) {
    return;
  }

  const float bx = e.cx - e.side * (e.rx + kBlushOutsetPx);
  const float by = e.cy + kBlushDrop * e.rx;
  const int x0 = floorInt(bx - kBlushRxPx);
  const int y0 = floorInt(by - kBlushRyPx);
  const int cols = floorInt(bx + kBlushRxPx) - x0 + 1;
  const int rows = floorInt(by + kBlushRyPx) - y0 + 1;

  float nx2[kBlushMaxCols];
  for (int k = 0; k < cols && k < kBlushMaxCols; k++) {
    const float nx = (center(x0 + k) - bx) / kBlushRxPx;
    nx2[k] = nx * nx;
  }

  uint16_t mask[kBlushMaxRows] = {};
  for (int r = 0; r < rows && r < kBlushMaxRows; r++) {
    const int y = y0 + r;
    const float ny = (center(y) - by) / kBlushRyPx;
    const float ny2 = ny * ny;
    for (int k = 0; k < cols && k < kBlushMaxCols; k++) {
      const int x = x0 + k;
      if (blushDot(x, y, nx2[k] + ny2, amount) && !nearLit(frame, x, y)) {
        mask[r] |= (uint16_t)(1u << k);
      }
    }
  }

  for (int r = 0; r < kBlushMaxRows; r++) {
    for (int k = 0; k < kBlushMaxCols; k++) {
      if (mask[r] & (1u << k)) {
        c.plot(x0 + k, y0 + r);
      }
    }
  }
}

// --------------------------------------------------------------- mouth ---

// Upper and lower edge of each mouth column.
struct MouthColumns {
  int x0;
  int count;
  float top[kMaxMouthColumns];
  float bottom[kMaxMouthColumns];
  bool inside[kMaxMouthColumns];

  bool has(int i) const { return i >= 0 && i < count && inside[i]; }

  // Extends edge[i] to the midpoints with its neighbours so steep curves stay
  // connected: toward the lower value when `low`, else the higher.
  float spanTo(const float* edge, int i, bool low) const {
    float v = edge[i];
    for (int n = i - 1; n <= i + 1; n += 2) {
      if (!has(n)) {
        continue;
      }
      const float mid = (edge[i] + edge[n]) * 0.5f;
      v = low ? fminf(v, mid) : fmaxf(v, mid);
    }
    return v;
  }
};

// Lip height at u (-1..1 across the mouth): a smile curve blended toward the
// two-bump "ω", plus an optional wobble.
float lipAt(const Mouth& m, float u, float omega) {
  const float single = 1.0f - u * u;
  const float twinU = 2.0f * fabsf(u) - 1.0f;  // each half as its own -1..1
  const float twin = 1.0f - twinU * twinU;
  const float lip = m.cy + m.curve * ((1.0f - omega) * single + omega * twin);
  return m.wave == 0.0f ? lip : lip + m.wave * sinf(u * kMouthWaveCycles * kPi);
}

bool mouthEdges(const Mouth& m, MouthColumns& out) {
  if (m.width < kMinMouthWidthPx) {
    return false;
  }

  const float halfW = m.width * 0.5f;
  out.x0 = floorInt(m.cx - halfW);
  out.count = floorInt(m.cx + halfW) - out.x0 + 1;
  if (out.count <= 0 || out.count > kMaxMouthColumns) {
    return false;
  }

  const float openUp = fmaxf(0.0f, m.openUp);
  const float openDown = fmaxf(0.0f, m.openDown);
  const float omega = clamp01(m.omega);

  for (int i = 0; i < out.count; i++) {
    const float u = (center(out.x0 + i) - m.cx) / halfW;
    out.inside[i] = fabsf(u) <= 1.0f;
    if (!out.inside[i]) {
      continue;
    }

    const float lip = lipAt(m, u, omega);
    const float rim = sqrtf(1.0f - u * u);  // opening is tallest at the center
    out.top[i] = lip - openUp * rim;
    out.bottom[i] = lip + openDown * rim;
  }
  return true;
}

void strokeMouth(Canvas& c, const MouthColumns& cols, bool hollow) {
  for (int i = 0; i < cols.count; i++) {
    if (!cols.inside[i]) {
      continue;
    }

    const int x = cols.x0 + i;
    const bool end = !cols.has(i - 1) || !cols.has(i + 1);
    if (!hollow || end) {
      c.column(x, floorInt(cols.spanTo(cols.top, i, true)), floorInt(cols.spanTo(cols.bottom, i, false)));
      continue;
    }

    c.column(x, floorInt(cols.spanTo(cols.top, i, true)), floorInt(cols.spanTo(cols.top, i, false)));
    c.column(x, floorInt(cols.spanTo(cols.bottom, i, true)), floorInt(cols.spanTo(cols.bottom, i, false)));
  }
}

void drawMouth(Canvas& c, const Mouth& m) {
  MouthColumns cols;
  if (mouthEdges(m, cols)) {
    strokeMouth(c, cols, m.hollow);
  }
}

// --------------------------------------------------------------- stars ---

void drawGlint(Canvas& c, const EyeShape& e, float glint) {
  if (glint < kMinGlint || e.rx < kMinEyeRadiusPx) {
    return;
  }

  c.star(
    e.cx - e.side * (e.rx + kGlintOutsetPx),
    e.cy - kGlintRise * e.rx,
    (int)(glint - kMinGlint)
  );
}

}  // namespace

bool pixelAt(const uint8_t* frame, int x, int y) {
  if (frame == nullptr || x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
    return false;
  }
  return (frame[y * kRowBytes + x / 8] & (0x80u >> (x & 7))) != 0;
}

bool render(const Face& face, uint8_t* frame, size_t frameBytes) {
  if (frame == nullptr || frameBytes < kFrameBytes) {
    return false;
  }

  memset(frame, 0, kFrameBytes);
  Canvas canvas(frame);

  drawEye(canvas, face.left);
  drawEye(canvas, face.right);
  drawBlush(canvas, frame, face.left, face.blush);
  drawBlush(canvas, frame, face.right, face.blush);

  drawMouth(canvas, face.mouth);
  drawGlint(canvas, face.left, face.glint);
  drawGlint(canvas, face.right, face.glint);
  return true;
}

}  // namespace kawaii
