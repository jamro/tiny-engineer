#pragma once

#include <cstdint>

// Synced to abort.wav: "Fine. I didn't want to finish it anyway." (2.5 s).
constexpr uint32_t ABORT_AUDIO_FINE_END_MS = 520;
constexpr uint32_t ABORT_AUDIO_DIDNT_WANT_END_MS = 1350;
constexpr uint32_t ABORT_AUDIO_FINISH_END_MS = 2050;
constexpr uint32_t ABORT_AUDIO_END_MS = 2500;

void startAbort();
void updateAbort(uint32_t now);
bool abortAudioStarted();
uint32_t abortAudioElapsed(uint32_t now);
