#pragma once

#include <stdint.h>

#include "display/eyes.h"

void resetImpactState();
void applyImpactOverlay(Eye& left, Eye& right, uint32_t now);
