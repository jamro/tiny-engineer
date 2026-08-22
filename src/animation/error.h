#pragma once

#include <cstdint>

// Synced to error.wav: "Uh-oh. Human, we have a problem." (~2.23 s).
constexpr uint32_t ERROR_AUDIO_UHOH_END_MS = 460;
constexpr uint32_t ERROR_AUDIO_HUMAN_END_MS = 1060;
constexpr uint32_t ERROR_AUDIO_PROBLEM_END_MS = 2140;
constexpr uint32_t ERROR_AUDIO_END_MS = 2220;

void startError();
void updateError(uint32_t now);
bool errorAudioStarted();
uint32_t errorAudioElapsed(uint32_t now);
