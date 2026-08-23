#pragma once

#include <cstdint>

#include "animation.h"

void setRgb(uint8_t r, uint8_t g, uint8_t b);
void setRgbForAnimation(AnimationId id, uint32_t nowMs);
void updateRgb(uint32_t nowMs);
void runRgbTest();
