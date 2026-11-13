#include "display/eyes/styles/kawaii/compose.h"

#include <cmath>

#include "display/eyes/styles/kawaii/util.h"

namespace kawaii {

namespace {

constexpr uint32_t kHopPeriodMs = 360;
constexpr uint32_t kQuiverPeriodMs = 70;  // a fast, small shiver
constexpr uint32_t kGlintPeriodMs = 560;
constexpr float kGlintBase = 1.6f;   // star size at full glint, mid-swing
constexpr float kGlintSwing = 1.4f;
constexpr float kHollowAbove = 0.5f;  // eased 0..1 → outline on/off

// -1..1 sine with the given period; modulo keeps float precision for large `now`.
float wave(uint32_t now, uint32_t periodMs) {
  return sinf(kTwoPi * (float)(now % periodMs) / (float)periodMs);
}

EyePose withHop(EyePose eye, float hop) {
  eye.cy += hop;
  return eye;
}

EyeShape eyeFrom(
  const EyePose& pose,
  int8_t side,
  const Expression& e,
  float happy,
  float open,
  const Pose& look
) {
  EyeShape eye = {};
  eye.kind = e.kind;
  eye.side = side;
  eye.cx = pose.cx;
  eye.cy = pose.cy;
  eye.rx = kEyeRadius * e.scale;
  eye.ry = eye.rx * pose.heightScale;
  eye.open = open;
  eye.happy = happy;
  eye.lid = e.lid;
  eye.tilt = e.tilt;
  // From the base radius, not the springy eye scale, so it never pulses.
  eye.pupil = kPupilRadius * kEyeRadius * e.pupil;
  eye.gazeX = look.gazeX + e.gazeX;
  eye.gazeY = look.gazeY + e.gazeY;
  eye.sparkle = e.sparkle;
  return eye;
}

Mouth mouthFrom(const EyePose& left, const EyePose& right, const Expression& e) {
  Mouth m = {};
  m.cx = (left.cx + right.cx) * 0.5f + e.mouthShift;
  m.cy = (left.cy + right.cy) * 0.5f + kMouthDrop;
  m.width = e.mouthWidth;
  m.curve = e.mouthCurve;
  m.omega = e.mouthOmega;
  m.openUp = e.mouthOpenUp;
  m.openDown = e.mouthOpenDown;
  m.wave = e.mouthWave;
  m.hollow = e.mouthHollow >= kHollowAbove;
  return m;
}

}  // namespace

void composeFace(const Pose& pose, const Animator& animator, uint32_t now, Face& out) {
  const Expression& e = animator.current();

  // Hop: bounces off the floor rather than swinging through it.
  const float hop = -fabsf(wave(now, kHopPeriodMs)) * e.bounce;
  const float open = pose.open * animator.kindOpen();

  const float shiver = e.quiver * wave(now, kQuiverPeriodMs);
  EyePose left = withHop(pose.left, hop);
  EyePose right = withHop(pose.right, hop);
  left.cx += shiver;
  right.cx += shiver;

  out.left = eyeFrom(left, 1, e, e.happyLeft, open, pose);
  out.right = eyeFrom(right, -1, e, e.happyRight, open, pose);
  out.mouth = mouthFrom(left, right, e);
  out.blush = e.blush;
  out.glint = e.glint * (kGlintBase + kGlintSwing * wave(now, kGlintPeriodMs));
}

}  // namespace kawaii
