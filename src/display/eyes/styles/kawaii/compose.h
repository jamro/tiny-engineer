#pragma once

#include <cstdint>

#include "display/eyes/styles/kawaii/animator.h"
#include "display/eyes/styles/kawaii/face.h"

namespace kawaii {

// Where the shared (classic) pose puts one eye.
struct EyePose {
  float cx;
  float cy;
  float heightScale;  // pose height / classic open height
};

struct Pose {
  EyePose left;
  EyePose right;
  float open;   // blink / sleep lid, 0..1
  float gazeX;  // where the choreography looks, -1..1 (left .. right)
  float gazeY;  // -1 up .. 1 down
};

constexpr float kEyeRadius = 11.0f;  // px at scale 1
constexpr float kPupilRadius = 0.38f;  // × eye radius at pupil 1
constexpr float kMouthDrop = 7.5f;   // mouth below the eye centers, px

// Builds the face to render from the pose, the eased expression, and time
// (hop, twinkle).
void composeFace(const Pose& pose, const Animator& animator, uint32_t now, Face& out);

}  // namespace kawaii
