#pragma once

#include <ESP_I2S.h>

extern I2SClass I2S;

void playTone(
  float frequency,
  int durationMs,
  int volume = 14000
);

void playSilence(int durationMs);
void runSoundTest();
