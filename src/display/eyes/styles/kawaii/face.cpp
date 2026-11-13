#include "display/eyes/styles/kawaii/face.h"

#include <cmath>
#include <cstring>

namespace kawaii {

namespace {

constexpr int kRowBytes = kWidth / 8;
constexpr int kMaxMouthColumns = 48;

// 4×4 ordered dither thresholds (0..15).
constexpr uint8_t kBayer[4][4] = {
  {0, 8, 2, 10},
  {12, 4, 14, 6},
  {3, 11, 1, 9},
  {15, 7, 13, 5},
};

float clamp01(float v) {
  if (v < 0.0f) {
    return 0.0f;
  }
  return v > 1.0f ? 1.0f : v;
}

int floorInt(float v) {
  return (int)floorf(v);
}

class Canvas {
 public:
  explicit Canvas(uint8_t* bits) : bits_(bits) {}

  void set(int x, int y, bool on = true) {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
      return;
    }

    uint8_t& byte = bits_[y * kRowBytes + x / 8];
    const uint8_t mask = (uint8_t)(0x80u >> (x & 7));
    byte = on ? (uint8_t)(byte | mask) : (uint8_t)(byte & ~mask);
  }

  void column(int x, int y0, int y1) {
    if (y0 > y1) {
      const int t = y0;
      y0 = y1;
      y1 = t;
    }
    for (int y = y0; y <= y1; y++) {
      set(x, y);
    }
  }

  void clearDisc(float cx, float cy, float r) {
    if (r < 1.1f) {  // smaller reads as a stray pixel, not a sparkle
      return;
    }

    const float r2 = r * r;
    for (int y = floorInt(cy - r); y <= floorInt(cy + r); y++) {
      for (int x = floorInt(cx - r); x <= floorInt(cx + r); x++) {
        const float dx = x + 0.5f - cx;
        const float dy = y + 0.5f - cy;
        if (dx * dx + dy * dy <= r2) {
          set(x, y, false);
        }
      }
    }
  }

  // Thick-ish plus-shaped twinkle star.
  void star(float cx, float cy, int arm) {
    if (arm < 0) {
      return;
    }

    const int x = floorInt(cx);
    const int y = floorInt(cy);
    set(x, y);
    for (int i = 1; i <= arm; i++) {
      set(x - i, y);
      set(x + i, y);
      set(x, y - i);
      set(x, y + i);
    }
  }

 private:
  uint8_t* bits_;
};

// ---------------------------------------------------------------- eyes ---

void drawClosedEye(Canvas& c, const EyeShape& e) {
  // "‿" lid line; "∩" when the eye was smiling as it closed.
  const float halfW = e.rx * 0.9f;
  const float sag = e.happy > 0.5f ? -2.2f : 2.2f;
  if (halfW < 1.0f) {
    return;
  }

  const int x0 = (int)ceilf(e.cx - halfW - 0.5f);
  const int x1 = floorInt(e.cx + halfW - 0.5f);
  for (int x = x0; x <= x1; x++) {
    const float u = (x + 0.5f - e.cx) / halfW;
    const float y = e.cy + sag * (1.0f - u * u) - sag * 0.5f - 0.5f;
    const int yi = floorInt(y);
    c.column(x, yi, yi + 1);
  }
}

void drawRoundEye(Canvas& c, const EyeShape& e, float ry) {
  const float rx = e.rx;
  const float happy = clamp01(e.happy);

  // Happy: a slightly smaller ellipse rises from below and carves the eye
  // into a "∩" crescent.
  // Wide while subtle so the whole lower edge lifts evenly; narrows to leave
  // a "∩" crescent at full smile.
  const float carveRy = ry * 0.95f;
  const float carveRx = rx * (1.3f - 0.35f * happy);
  const float carveTop = e.cy + ry - happy * 1.55f * ry;
  const float carveCy = carveTop + carveRy;

  // Upper lid: a straight cut, lowered by `lid` and tilted by `tilt`.
  const float lidBase = e.cy - ry + clamp01(e.lid) * 1.2f * ry;
  const float lidSlope = e.tilt * e.side * ry / rx;

  for (int y = floorInt(e.cy - ry); y <= floorInt(e.cy + ry); y++) {
    const float py = y + 0.5f;
    const float t = (py - e.cy) / ry;
    if (t * t >= 1.0f) {
      continue;
    }

    const float halfW = rx * sqrtf(1.0f - t * t);
    const int x0 = (int)ceilf(e.cx - halfW - 0.5f);
    const int x1 = floorInt(e.cx + halfW - 0.5f);

    float carveHalfW = -1.0f;
    if (happy > 0.001f) {
      const float ct = (py - carveCy) / carveRy;
      if (ct * ct < 1.0f) {
        carveHalfW = carveRx * sqrtf(1.0f - ct * ct);
      }
    }

    for (int x = x0; x <= x1; x++) {
      const float dx = x + 0.5f - e.cx;
      if (fabsf(dx) < carveHalfW) {
        continue;
      }
      if (py < lidBase + lidSlope * dx) {
        continue;
      }
      c.set(x, y);
    }
  }

  // Sparkles: one big and one small reflection, same light for both eyes.
  // They fade as the eye squints so a thin crescent is never punched through.
  const float fit = fminf(rx, ry);
  const float strength = e.sparkle * (1.0f - happy) * (1.0f - clamp01(e.lid));
  if (ry < 5.0f || strength <= 0.05f) {
    return;
  }

  // Keep the big highlight under a lowered lid instead of notching it.
  const float bigR = strength * 0.3f * fit;
  const float bigX = e.cx - 0.36f * rx;
  float bigY = e.cy - 0.36f * ry;
  const float lidAtHighlight = lidBase + lidSlope * (bigX - e.cx);
  if (bigY - bigR < lidAtHighlight + 1.5f) {
    bigY = lidAtHighlight + 1.5f + bigR;
  }
  c.clearDisc(bigX, bigY, bigR);
  c.clearDisc(e.cx + 0.42f * rx, e.cy + 0.4f * ry, strength * 0.17f * fit);
}

void drawChevron(Canvas& c, const EyeShape& e, float ry) {
  // ">" for the screen-left eye, "<" for the right: points at the nose.
  const float h = ry * 0.78f;
  const float w = e.rx * 0.95f;
  constexpr float kHalfStroke = 1.5f;

  for (int y = floorInt(e.cy - h); y <= floorInt(e.cy + h); y++) {
    const float a = fabsf(y + 0.5f - e.cy) / h;
    if (a > 1.0f) {
      continue;
    }

    const float xc = e.cx + e.side * (w * 0.5f - w * a);
    for (int x = floorInt(xc - kHalfStroke); x <= floorInt(xc + kHalfStroke); x++) {
      if (fabsf(x + 0.5f - xc) <= kHalfStroke) {
        c.set(x, y);
      }
    }
  }
}

void drawCross(Canvas& c, const EyeShape& e, float ry) {
  const float h = ry * 0.68f;
  const float w = e.rx * 0.62f;
  constexpr float kHalfStroke = 1.4f;

  for (int y = floorInt(e.cy - h); y <= floorInt(e.cy + h); y++) {
    const float d = (y + 0.5f - e.cy) / h;
    if (d < -1.0f || d > 1.0f) {
      continue;
    }

    const float xa = e.cx + d * w;
    const float xb = e.cx - d * w;
    for (int x = floorInt(e.cx - w - 2.0f); x <= floorInt(e.cx + w + 2.0f); x++) {
      const float px = x + 0.5f;
      if (fabsf(px - xa) <= kHalfStroke || fabsf(px - xb) <= kHalfStroke) {
        c.set(x, y);
      }
    }
  }
}

void drawHeart(Canvas& c, const EyeShape& e, float ry) {
  // (X² + Y² − 1)³ − X²Y³ ≤ 0, scaled to the eye box.
  const float sx = e.rx * 0.88f;
  const float sy = ry * 0.88f;

  for (int y = floorInt(e.cy - ry * 1.05f); y <= floorInt(e.cy + ry * 1.05f); y++) {
    const float Y = (e.cy - (y + 0.5f)) / sy + 0.12f;
    for (int x = floorInt(e.cx - e.rx * 1.05f); x <= floorInt(e.cx + e.rx * 1.05f); x++) {
      const float X = (x + 0.5f - e.cx) / sx;
      const float a = X * X + Y * Y - 1.0f;
      if (a * a * a - X * X * Y * Y * Y <= 0.0f) {
        c.set(x, y);
      }
    }
  }

  if (ry >= 5.0f && e.sparkle > 0.05f) {
    c.clearDisc(
      e.cx - 0.45f * e.rx,
      e.cy - 0.42f * ry,
      e.sparkle * 0.18f * fminf(e.rx, ry)
    );
  }
}

void drawEye(Canvas& c, const EyeShape& e) {
  if (e.rx < 1.0f) {
    return;
  }

  const float ry = e.ry * clamp01(e.open);
  // Line shapes break into dashes when squashed: hold the lid line until
  // they have room.
  const float closedBelow = e.kind == EyeKind::Round ? 1.4f : 0.4f * e.ry;
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

bool nearLit(const uint8_t* eyesOnly, int x, int y) {
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (pixelAt(eyesOnly, x + dx, y + dy)) {
        return true;
      }
    }
  }
  return false;
}

void drawBlush(Canvas& c, const uint8_t* eyesOnly, const EyeShape& e, float amount) {
  if (amount <= 0.02f || e.rx < 1.0f) {
    return;
  }

  // Under the outer corner, from the open-eye geometry so it holds still
  // through blinks.
  constexpr float kRx = 5.5f;
  constexpr float kRy = 2.4f;
  const float bx = e.cx - e.side * (e.rx + 1.5f);
  const float by = e.cy + 0.6f * e.rx;

  for (int y = floorInt(by - kRy); y <= floorInt(by + kRy); y++) {
    for (int x = floorInt(bx - kRx); x <= floorInt(bx + kRx); x++) {
      const float nx = (x + 0.5f - bx) / kRx;
      const float ny = (y + 0.5f - by) / kRy;
      const float d2 = nx * nx + ny * ny;
      if (d2 >= 1.0f) {
        continue;
      }

      // Soft edge; full blush peaks at a 50 % checkerboard.
      const float level = amount * clamp01((1.0f - d2) * 2.2f);
      if (kBayer[y & 3][x & 3] >= level * 8.0f) {
        continue;
      }
      if (nearLit(eyesOnly, x, y)) {
        continue;
      }
      c.set(x, y);
    }
  }
}

// --------------------------------------------------------------- mouth ---

void drawMouth(Canvas& c, const Mouth& m) {
  const float halfW = m.width * 0.5f;
  if (halfW < 0.5f) {
    return;
  }

  const int x0 = floorInt(m.cx - halfW);
  const int count = floorInt(m.cx + halfW) - x0 + 1;
  if (count <= 0 || count > kMaxMouthColumns) {
    return;
  }

  float top[kMaxMouthColumns];
  float bottom[kMaxMouthColumns];
  bool inside[kMaxMouthColumns];
  const float openUp = fmaxf(0.0f, m.openUp);
  const float openDown = fmaxf(0.0f, m.openDown);
  const float omega = clamp01(m.omega);

  for (int i = 0; i < count; i++) {
    const float u = (x0 + i + 0.5f - m.cx) / halfW;
    inside[i] = u >= -1.0f && u <= 1.0f;
    if (!inside[i]) {
      continue;
    }

    const float single = 1.0f - u * u;
    const float twin = 1.0f - (2.0f * fabsf(u) - 1.0f) * (2.0f * fabsf(u) - 1.0f);
    const float lip = m.cy
      + m.curve * ((1.0f - omega) * single + omega * twin)
      + m.wave * sinf(u * 3.0f * 3.14159265f);
    const float rim = sqrtf(single);
    top[i] = lip - openUp * rim;
    bottom[i] = lip + openDown * rim;
  }

  // Each column spans to the midpoint of its neighbours so steep curves stay
  // connected.
  auto spanTo = [&](const float* edge, int i, bool low) {
    float v = edge[i];
    for (int n = i - 1; n <= i + 1; n += 2) {
      if (n < 0 || n >= count || !inside[n]) {
        continue;
      }
      const float mid = (edge[i] + edge[n]) * 0.5f;
      v = low ? fminf(v, mid) : fmaxf(v, mid);
    }
    return v;
  };

  for (int i = 0; i < count; i++) {
    if (!inside[i]) {
      continue;
    }

    const int x = x0 + i;
    const bool end = i == 0 || i == count - 1 || !inside[i - 1] || !inside[i + 1];

    if (!m.hollow || end) {
      c.column(x, floorInt(spanTo(top, i, true)), floorInt(spanTo(bottom, i, false)));
      continue;
    }

    c.column(x, floorInt(spanTo(top, i, true)), floorInt(spanTo(top, i, false)));
    c.column(x, floorInt(spanTo(bottom, i, true)), floorInt(spanTo(bottom, i, false)));
  }
}

// --------------------------------------------------------------- stars ---

void drawGlint(Canvas& c, const EyeShape& e, float glint) {
  if (glint < 0.3f || e.rx < 1.0f) {
    return;
  }

  c.star(e.cx - e.side * (e.rx + 4.5f), e.cy - 0.8f * e.rx, (int)(glint - 0.3f));
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

  // Blush keeps a one-pixel gap from whatever the eyes drew.
  static uint8_t eyesOnly[kFrameBytes];
  memcpy(eyesOnly, frame, kFrameBytes);
  drawBlush(canvas, eyesOnly, face.left, face.blush);
  drawBlush(canvas, eyesOnly, face.right, face.blush);

  drawMouth(canvas, face.mouth);
  drawGlint(canvas, face.left, face.glint);
  drawGlint(canvas, face.right, face.glint);
  return true;
}

}  // namespace kawaii
