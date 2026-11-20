#pragma once

#include <stdint.h>

namespace anim {

struct EasedMove {
  float from;
  float to;
  uint32_t startMs;
  uint32_t durationMs;
  bool active;
};

// Pose that looks at the user, in -1..1 units. The saved "face me" pose when
// the user set one, else the talking rest pose (head 1/7, neck 0, body 0).
struct FacePose {
  float head;
  float neck;
  float body;
};

FacePose facePose();

float randUnit();
bool randChance(uint32_t percent);
uint32_t randRangeMs(uint32_t lo, uint32_t hi);
float easeInOutCubic(float t);
float lerp(float a, float b, float t);

void beginEasedMove(
  EasedMove& move,
  float from,
  float to,
  uint32_t startMs,
  uint32_t durationMs
);
float easedMoveValue(const EasedMove& move, uint32_t now);
bool easedMoveDone(const EasedMove& move, uint32_t now);
float easedLerp(
  float from,
  float to,
  uint32_t startMs,
  uint32_t durationMs,
  uint32_t now
);

void stopAnimServos();
void parkTorso(float speedDegS);
void parkHands(float speedDegS);
void parkForTransition();
bool isTransitionParkComplete();
void parkHandsAndBody();
void parkNonePose();
void parkSleepPose();
void snapHeadToRangeHigh(float highNorm);

void logServoSnapshot(const char* tag);

}  // namespace anim
