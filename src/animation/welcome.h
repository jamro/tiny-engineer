#pragma once

#include <stdint.h>

// Synced to welcome.wav: "Hello, human. … What are we building today?" (~2.68 s).
constexpr uint32_t WELCOME_AUDIO_GREETING_END_MS = 800;
constexpr uint32_t WELCOME_AUDIO_PAUSE_END_MS = 1340;
constexpr uint32_t WELCOME_AUDIO_BLINK_START_MS = 1020;
constexpr uint32_t WELCOME_AUDIO_BLINK_END_MS = 1120;
constexpr uint32_t WELCOME_AUDIO_QUESTION_END_MS = 2400;
constexpr uint32_t WELCOME_AUDIO_END_MS = 2680;

void startWelcome();
void updateWelcome(uint32_t now);
bool welcomeAudioStarted();
uint32_t welcomeAudioElapsed(uint32_t now);
