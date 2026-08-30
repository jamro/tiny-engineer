#pragma once

#include <stdint.h>

void startWakeup(uint32_t nowMs, bool moveServos);
void updateWakeup(uint32_t nowMs);
bool wakeupFinished();
