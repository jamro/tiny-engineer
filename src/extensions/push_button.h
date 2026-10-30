#pragma once

#include <cstdint>

// Extension-port push button. Off by default (settingsPushButton).
// Pin comes from settingsPushButtonPin (default GP20).

void pushButtonApply(bool enabled, uint8_t pin);

// Applies settings with pin-claim conflict check: contested pins stay idle + log.
void pushButtonApplyFromSettings();

void pushButtonPoll();
