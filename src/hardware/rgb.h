#pragma once

#include <cstdint>

#include "animation.h"

// Boot-failure blink codes: how many red flashes before the long gap.
constexpr uint8_t RGB_CODE_PCA9685 = 1;
constexpr uint8_t RGB_CODE_I2S = 2;

void setRgb(uint8_t r, uint8_t g, uint8_t b);

// Blinks red `blinks` times, pauses, and repeats forever. Never returns.
[[noreturn]] void haltWithRgbCode(uint8_t blinks);
void setRgbForAnimation(AnimationId id, uint32_t nowMs);
void updateRgb(uint32_t nowMs);
void runRgbTest();
