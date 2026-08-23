#pragma once

#include <cstdint>

// Synced to attention.wav: "pst... human.... you might want to take a look" (~2.96 s).
constexpr uint32_t ATTENTION_AUDIO_PST_END_MS = 640;
constexpr uint32_t ATTENTION_AUDIO_HUMAN_END_MS = 1540;
constexpr uint32_t ATTENTION_AUDIO_BLINK_START_MS = 720;
constexpr uint32_t ATTENTION_AUDIO_BLINK_END_MS = 780;
constexpr uint32_t ATTENTION_AUDIO_END_MS = 2960;

void startAttention();
void updateAttention(uint32_t now);
bool attentionAudioStarted();
uint32_t attentionAudioElapsed(uint32_t now);
