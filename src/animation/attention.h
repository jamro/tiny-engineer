#pragma once

#include <cstdint>

// Synced to attention.wav: "Your turn, human." (~1.07 s).
constexpr uint32_t ATTENTION_AUDIO_TURN_END_MS = 700;
constexpr uint32_t ATTENTION_AUDIO_BLINK_START_MS = 620;
constexpr uint32_t ATTENTION_AUDIO_BLINK_END_MS = 680;
constexpr uint32_t ATTENTION_AUDIO_END_MS = 1070;

void startAttention();
void updateAttention(uint32_t now);
bool attentionAudioStarted();
uint32_t attentionAudioElapsed(uint32_t now);
