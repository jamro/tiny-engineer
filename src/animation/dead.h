#pragma once

#include <cstdint>

// Eyes: squeeze nearly shut after warning, then brief hold before X X.
constexpr uint32_t DEAD_SQUEEZE_MS = 300;
constexpr uint32_t DEAD_PAUSE_MS = 200;

void startDead();
void updateDead(uint32_t now);
bool deadShowingX();
bool deadIsSqueezing();
bool deadIsPausedShut();
uint32_t deadSqueezeStartedMs();
