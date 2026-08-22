#pragma once

#include <stdint.h>

namespace anim {

float randUnit();
bool randChance(uint32_t percent);
uint32_t randRangeMs(uint32_t lo, uint32_t hi);
float easeInOutCubic(float t);

void stopAnimServos();
void parkTorso(float speedDegS);
void parkHands(float speedDegS);
void parkForTransition();
bool isTransitionParkComplete();
void parkHandsAndBody();
void parkNonePose();
void snapHeadToRangeHigh(float highDeg);

void logServoSnapshot(const char* tag);

}  // namespace anim
