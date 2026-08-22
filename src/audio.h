#pragma once

#include <ESP_I2S.h>

extern I2SClass I2S;

bool initAudioStorage();
void playTone(
  float frequency,
  int durationMs,
  int volume = 14000
);

void playSilence(int durationMs);
bool startBellPlayback();
bool updateBellPlayback();
void stopBellPlayback();
bool playBell();
void runSoundTest();
