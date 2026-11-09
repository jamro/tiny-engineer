#pragma once

#include <cstdint>

#include "animation.h"

void initSleep();
void prepareSleepWakePose();
void requestSleep(uint32_t now);
void onAnimationApplied(AnimationId id, uint32_t now);
// Restarts the idle countdown, e.g. while a clip plays over the idle pose.
void noteActivity(uint32_t now);
void updateSleep(uint32_t now);
bool isSleeping();
// True while asleep or entering sleep (Closing / Sleeping).
bool sleepAllowsWakeup();
