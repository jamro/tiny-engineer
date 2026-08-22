#pragma once

#include "display/eyes.h"

namespace eyes {

constexpr int EYE_CORNER_RADIUS = 3;
constexpr float BLINK_CLOSED_AMOUNT = 0.12f;
constexpr uint32_t REDRAW_INTERVAL_MS = 33;
constexpr uint32_t DOUBLE_BLINK_CHANCE_PERCENT = 22;

constexpr Eye DEFAULT_LEFT = {20, 9, 24, 14};
constexpr Eye DEFAULT_RIGHT = {84, 9, 24, 14};
constexpr int16_t EYE_CENTER_Y =
  DEFAULT_LEFT.y + DEFAULT_LEFT.height / 2;

}  // namespace eyes
