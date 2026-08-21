#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#include "pins.h"
#include "oled.h"
#include "audio.h"

I2SClass I2S;

void playTone(
  float frequency,
  int durationMs,
  int volume
) {
  constexpr int FRAMES = 512;

  int16_t buffer[FRAMES * 2];

  float phase = 0.0f;

  const float phaseStep =
    2.0f * PI * frequency / SAMPLE_RATE;

  const int totalFrames =
    SAMPLE_RATE * durationMs / 1000;

  int framesPlayed = 0;

  while (framesPlayed < totalFrames) {
    const int framesThisTime =
      min(
        FRAMES,
        totalFrames - framesPlayed
      );

    for (int i = 0; i < framesThisTime; i++) {
      const int16_t sample =
        (int16_t)(
          sin(phase) * volume
        );

      phase += phaseStep;

      if (phase >= 2.0f * PI) {
        phase -= 2.0f * PI;
      }

      buffer[i * 2] = sample;
      buffer[i * 2 + 1] = sample;
    }

    I2S.write(
      (uint8_t*)buffer,
      framesThisTime *
      2 *
      sizeof(int16_t)
    );

    framesPlayed += framesThisTime;
  }
}

void playSilence(int durationMs) {
  constexpr int FRAMES = 512;

  int16_t buffer[FRAMES * 2] = {0};

  const int totalFrames =
    SAMPLE_RATE * durationMs / 1000;

  int framesPlayed = 0;

  while (framesPlayed < totalFrames) {
    const int framesThisTime =
      min(
        FRAMES,
        totalFrames - framesPlayed
      );

    I2S.write(
      (uint8_t*)buffer,
      framesThisTime *
      2 *
      sizeof(int16_t)
    );

    framesPlayed += framesThisTime;
  }
}

void runSoundTest() {
  Serial.println();
  Serial.println("==========================");
  Serial.println("SOUND TEST");
  Serial.println("==========================");

  showOledText(
    "SOUND TEST",
    "500 Hz"
  );

  playTone(500, 120);
  playSilence(50);

  showOledText(
    "SOUND TEST",
    "700 Hz"
  );

  playTone(700, 120);
  playSilence(50);

  showOledText(
    "SOUND TEST",
    "1000 Hz"
  );

  playTone(1000, 220);
  playSilence(100);

  showOledText(
    "SOUND TEST",
    "OK"
  );

  Serial.println("Sound OK");

  delay(500);
}
