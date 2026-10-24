#pragma once

#include <ESP_I2S.h>

extern I2SClass I2S;

bool initAudioStorage();
// Volume-scaled, duplicated to both I2S slots. Blocks until the DMA buffer accepts it.
void writeMonoToSpeaker(const int16_t* samples, size_t count);
void playTone(
  float frequency,
  int durationMs
);

void playSilence(int durationMs);
void stopAllWavPlayback();
// Starts bell/click for push-button events when free; false if none, busy, or start failed.
bool tryStartButtonEventSound(const char* sound);
bool startBellPlayback();
bool updateBellPlayback();
void stopBellPlayback();
bool playBell();
bool startWelcomePlayback();
bool updateWelcomePlayback();
void stopWelcomePlayback();
bool playWelcome();
bool startAttentionPlayback();
bool updateAttentionPlayback();
void stopAttentionPlayback();
bool playAttention();
bool startErrorPlayback();
bool updateErrorPlayback();
void stopErrorPlayback();
bool playError();
bool startAbortPlayback();
bool updateAbortPlayback();
void stopAbortPlayback();
bool playAbort();
bool startDeadPlayback();
bool updateDeadPlayback();
void stopDeadPlayback();
bool playDead();
void runSoundTest();
