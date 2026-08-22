#pragma once

#include <stdint.h>

namespace anim {

float randUnit();
bool randChance(uint32_t percent);
uint32_t randRangeMs(uint32_t lo, uint32_t hi);
float easeInOutCubic(float t);

void stopAnimServos();
void parkHandsAndBody();
void parkNonePose();
void snapHeadToRangeHigh(float highDeg);

}  // namespace anim
