#pragma once

#include <cstdint>

void initSleep();
void touchApiActivity();
void updateSleep(uint32_t now);
bool isSleeping();
