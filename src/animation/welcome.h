#pragma once

#include <stdint.h>

constexpr uint32_t WELCOME_AUDIO_WELCOME_END_MS = 730;
constexpr uint32_t WELCOME_AUDIO_PAUSE_END_MS = 1100;
constexpr uint32_t WELCOME_AUDIO_LOGIN_END_MS = 1600;
constexpr uint32_t WELCOME_AUDIO_ACCEPTED_END_MS = 2330;

void startWelcome();
void updateWelcome(uint32_t now);
bool welcomeAudioStarted();
uint32_t welcomeAudioElapsed(uint32_t now);
