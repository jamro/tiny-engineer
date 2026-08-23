#pragma once

#include <cstdint>

#include "animation.h"

void initSleep();
void onAnimationApplied(AnimationId id, uint32_t now);
void updateSleep(uint32_t now);
bool isSleeping();
