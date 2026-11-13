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

// Pupil: a dark disc that follows the gaze. It always keeps a rim of eye
// around it, so lids and smiles never notch the outline; a squint shrinks it.
constexpr float kPupilRimPx = 1.5f;
constexpr float kMinPupilRadiusPx = 1.6f;  // smaller is skipped (blinks, ^ ^)
constexpr float kPupilShrink = 0.85f;  // per try when a squint has no room
// Catch-light: a white dot inside the pupil, up and to the left in both eyes.
// White on black, so it never reads as a second pupil.
constexpr float kCatchLightRadius = 0.36f;  // × pupil radius at sparkle 1
constexpr float kCatchLightOffset = -0.3f;  // × pupil radius, on both axes
constexpr float kMinCatchLightRadiusPx = 0.9f;
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
    const float r2 = r * r;
    const int lo = floorInt(cx - r);
    const int hi = floorInt(cx + r);
    for (int y = floorInt(cy - r); y <= floorInt(cy + r); y++) {
      const float dy = center(y) - cy;
      const float rest = r2 - dy * dy;
      if (rest < 0.0f) {
        continue;
      }
      const auto inside = [&](int x) {
        const float dx = center(x) - cx;
        return dx * dx + dy * dy <= r2;
      };
      const Span span = runAround(cx, sqrtf(rest), lo, hi, inside);
      for (int x = span.first; x <= span.last; x++) {
        set(x, y, on);
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

// The visible part of a round eye: inside the ellipse, below the lid, and
// outside the happy carve.
class EyeArea {
 public:
  EyeArea(const EyeShape& e, float ry, const HappyCarve& carve, const LidCut& lid)
    : e_(e), ry_(ry), carve_(carve), lid_(lid) {}

  float cx() const { return e_.cx; }
  float cy() const { return e_.cy; }
  float rx() const { return e_.rx; }
  float ry() const { return ry_; }
  float lidAt(float x) const { return lid_.edgeAt(x); }
  float bottom() const { return fminf(e_.cy + ry_, carve_.top(e_.cy + ry_)); }

  bool contains(float x, float y) const {
    const float nx = (x - e_.cx) / e_.rx;
    const float ny = (y - e_.cy) / ry_;
    return nx * nx + ny * ny <= 1.0f && !lid_.hides(x, y) &&
      fabsf(x - e_.cx) >= carve_.halfWidthAt(y);
  }

  // True if a disc of radius r at (x, y), plus the rim, is all visible eye.
  bool fits(float x, float y, float r) const {
    const float reach = r + kPupilRimPx;
    for (const Dir& d : kFitDirs) {
      if (!contains(x + reach * d.x, y + reach * d.y)) {
        return false;
      }
    }
    return true;
  }

 private:
  struct Dir {
    float x;
    float y;
  };
  // Twelve directions around a circle, 30° apart.
  static constexpr Dir kFitDirs[] = {
    {1.0f, 0.0f}, {0.866f, 0.5f}, {0.5f, 0.866f}, {0.0f, 1.0f},
    {-0.5f, 0.866f}, {-0.866f, 0.5f}, {-1.0f, 0.0f}, {-0.866f, -0.5f},
    {-0.5f, -0.866f}, {0.0f, -1.0f}, {0.5f, -0.866f}, {0.866f, -0.5f},
  };

  const EyeShape& e_;
  float ry_;
  const HappyCarve& carve_;
  const LidCut& lid_;
};

// Where a pupil of radius r looks: the gaze scaled to the room left around
// it, then clear of the lid and carve and back inside the travel ellipse at
// that height. A second pass settles a tilted lid, which is lower at a side.
void aimPupil(const EyeArea& a, float gx, float gy, float r, float& px, float& py) {
  const float travelX = fmaxf(0.0f, a.rx() - r - kPupilRimPx);
  const float travelY = fmaxf(0.0f, a.ry() - r - kPupilRimPx);
  px = a.cx() + gx * travelX;
  py = a.cy() + gy * travelY;

  for (int pass = 0; pass < 2; pass++) {
    const float lidLow = fmaxf(a.lidAt(px - r), a.lidAt(px + r));
    const float lo = fmaxf(a.cy() - travelY, lidLow + r + kPupilRimPx);
    const float hi = fminf(a.cy() + travelY, a.bottom() - r - kPupilRimPx);
    py = lo > hi ? (lo + hi) * 0.5f : fminf(fmaxf(py, lo), hi);

    const float v = travelY > 0.0f ? (py - a.cy()) / travelY : 0.0f;
    const float reach = travelX * sqrtf(fmaxf(0.0f, 1.0f - v * v));
    px = a.cx() + fminf(fmaxf(px - a.cx(), -reach), reach);
  }
}

// Places the pupil as asked when it fits; otherwise slides it toward the
// middle, then shrinks it. False when even the smallest pupil has no room.
bool placePupil(const EyeArea& a, float gx, float gy, float& r, float& px, float& py) {
  constexpr float kPulls[] = {1.0f, 0.5f, 0.0f};  // share of the sideways look kept
  for (; r >= kMinPupilRadiusPx; r *= kPupilShrink) {
    aimPupil(a, gx, gy, r, px, py);
    for (float pull : kPulls) {
      const float x = a.cx() + (px - a.cx()) * pull;
      if (a.fits(x, py, r)) {
        px = x;
        return true;
      }
    }
  }
  return false;
}

void drawPupil(Canvas& c, const EyeShape& e, const EyeArea& area) {
  float r = fminf(e.pupil, e.rx - kPupilRimPx);
  float gx = 0.0f;
  float gy = 0.0f;
  unitGaze(e, gx, gy);
  float px = 0.0f;
  float py = 0.0f;
  if (!placePupil(area, gx, gy, r, px, py)) {
    return;
  }
  c.disc(px, py, r, false);

  const float light = e.sparkle * kCatchLightRadius * r;
  if (light >= kMinCatchLightRadiusPx) {
    c.disc(px + kCatchLightOffset * r, py + kCatchLightOffset * r, light, true);
  }
}

void drawRoundEye(Canvas& c, const EyeShape& e, float ry) {
  const float happy = clamp01(e.happy);
  const HappyCarve carve(e, ry, happy);
  const LidCut lid(e, ry);

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
  }

  drawPupil(c, e, EyeArea(e, ry, carve, lid));
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
