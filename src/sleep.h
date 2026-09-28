#pragma once

#include <cstdint>

#include "animation.h"

void initSleep();
void prepareSleepWakePose();
void requestSleep(uint32_t now);
void onAnimationApplied(AnimationId id, uint32_t now);
void updateSleep(uint32_t now);
bool isSleeping();
