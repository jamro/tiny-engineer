#pragma once

#include <stdint.h>

// Where the waiting head looks, for the eyes to follow.
// x: -1 = the robot's right, +1 = the robot's left.
// y: -1 = down, +1 = up.
// sinceMs changes each time the head picks a new corner.
struct WaitingGaze {
  float x;
  float y;
  uint32_t sinceMs;
};

void startWaiting(uint32_t animationStartedMs);
void updateWaiting(uint32_t now);
WaitingGaze waitingGaze();
