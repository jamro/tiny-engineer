#pragma once

#include <stdint.h>

namespace anim {

struct SleepNodOffPose {
  float open;
  float head;
};

// Nod-off over SLEEP_NOD_OFF_MS. `fromOpen` / `fromHead` are the pose at t=0.
SleepNodOffPose sleepNodOffPose(
  uint32_t elapsedMs,
  float fromOpen,
  float fromHead
);

}  // namespace anim

void startSleepAnim(uint32_t nowMs);
void updateSleepAnim(uint32_t nowMs);
