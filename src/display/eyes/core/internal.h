#pragma once

#include <stdint.h>

#include "display/eyes.h"

namespace eyes {

uint32_t modeStartedMs();
void setModeStartedMs(uint32_t ms);

EyeMode currentEyeMode();
void setCurrentEyeMode(EyeMode mode);

bool forceRedraw();
void setForceRedraw(bool value);
void requestForceRedraw();

uint32_t lastDrawMs();
void setLastDrawMs(uint32_t ms);

bool eyesRunning();
void setEyesRunning(bool active);

}  // namespace eyes
