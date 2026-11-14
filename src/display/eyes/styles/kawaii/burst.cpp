#include "display/eyes/styles/kawaii/burst.h"

#include <cmath>

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

namespace {

// One star per eye for each launch, mirrored left and right. `angle` is its
// heading from straight out (radians; - is up), `spin` how fast it curls
// (rad/s, same sign as angle so it curls away from the eye), `dy` where on
// the eye's outer edge it starts (px from center).
struct Launch {
  uint16_t delayMs;
  float angle;
  float spin;
  float dy;
};
constexpr Launch kLaunches[] = {
  {0, -0.45f, -1.6f, -5.0f},
  {30, 0.40f, 1.6f, 4.0f},
  {110, -0.15f, -1.2f, -1.0f},
  {200, 0.75f, 1.8f, 6.0f},
  {320, -0.80f, -1.8f, -6.0f},
  {480, 0.10f, 1.4f, 2.0f},
};

// The spiral out: distance from the launch point after t seconds.
constexpr float kSpeedPxS = 28.0f;
constexpr float kAccelPxS2 = 45.0f;
constexpr float kLaunchGapPx = 1.0f;  // out from the eye's outer edge

constexpr int kTrailMax = 5;         // dots behind a star at full ring
constexpr float kTrailStepS = 0.025f;
constexpr float kBigStarRing = 0.25f;  // louder: a plus; quieter: one pixel
constexpr float kMsPerS = 1000.0f;

float travelPx(float t) {
  return kSpeedPxS * t + kAccelPxS2 * t * t;
}

bool onScreen(float x, float y) {
  return x >= -1.0f && x <= kWidth && y >= -1.0f && y <= kHeight;
}

void add(Face& face, float x, float y, int8_t arm) {
  if (face.sparkCount >= kMaxSparks || !onScreen(x, y)) {
    return;
  }
  face.sparks[face.sparkCount++] = {(int16_t)floorf(x), (int16_t)floorf(y), arm};
}

// One star and its trail. The trail steps back along the curl with a fixed
// small rotation, so only the head needs sin and cos.
void addStar(Face& face, const EyeShape& eye, const Launch& l, float t, float ring) {
  const float side = -(float)eye.side;  // screen-left eye flies left
  const float ox = eye.cx + side * (eye.rx + kLaunchGapPx);
  const float oy = eye.cy + l.dy;

  const float a = l.angle + l.spin * t;
  float c = cosf(a);
  float s = sinf(a);
  add(face, ox + side * travelPx(t) * c, oy + travelPx(t) * s, ring >= kBigStarRing ? 1 : 0);

  const int trail = (int)(kTrailMax * sqrtf(ring) + 0.5f);
  const float step = -l.spin * kTrailStepS;  // small: a short series is exact enough
  const float stepCos = 1.0f - step * step * 0.5f;
  const float stepSin = step - step * step * step / 6.0f;
  for (int k = 1; k <= trail; k++) {
    const float tk = t - k * kTrailStepS;
    if (tk <= 0.0f) {
      break;
    }
    const float nc = c * stepCos - s * stepSin;
    s = s * stepCos + c * stepSin;
    c = nc;
    add(face, ox + side * travelPx(tk) * c, oy + travelPx(tk) * s, 0);
  }
}

}  // namespace

void addStarBurst(Face& face, uint32_t sinceDingMs, float ring) {
  if (ring <= 0.0f) {
    return;
  }
  for (const Launch& l : kLaunches) {
    if (sinceDingMs < l.delayMs) {
      continue;
    }
    const float t = (float)(sinceDingMs - l.delayMs) / kMsPerS;
    addStar(face, face.left, l, t, ring);
    addStar(face, face.right, l, t, ring);
  }
}

}  // namespace kawaii
