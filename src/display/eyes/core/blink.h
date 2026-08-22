#pragma once

#include <stdint.h>

#include "display/eyes.h"

void blinkBeginIdle(uint32_t now);
void blinkSetNextBlinkMs(uint32_t when);
void blinkSetOpenAmount(float amount);
float blinkOpenAmount();
void blinkResetCounters();
void blinkAdvance(uint32_t now);
bool blinkIsIdle();
void blinkScheduleSoon(uint32_t now, uint32_t loMs, uint32_t hiMs);
void blinkOnSleepOpenComplete(uint32_t now);
