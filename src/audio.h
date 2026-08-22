#pragma once

#include <ESP_I2S.h>

extern I2SClass I2S;

bool initAudioStorage();
void playTone(
  float frequency,
  int durationMs
);

void playSilence(int durationMs);
void stopAllWavPlayback();
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
void runSoundTest();
