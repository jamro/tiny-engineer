#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <LittleFS.h>
#include <math.h>

#include "audio/wav_stream.h"
#include "pins.h"
#include "display/oled.h"
#include "audio.h"

I2SClass I2S;

namespace {

bool audioStorageReady = false;

constexpr const char* kAudioPartition = "spiffs";

void logAudioStorageContents() {
  File root = LittleFS.open("/");

  if (!root || !root.isDirectory()) {
    Serial.println("LittleFS root unavailable");
    return;
  }

  Serial.println("LittleFS files:");

  File entry = root.openNextFile();

  while (entry) {
    Serial.print("  ");
    Serial.println(entry.name());
    entry.close();
    entry = root.openNextFile();
  }

  root.close();
}

bool playWavBlocking(
  WavClip clip,
  const char* line2
) {
  const char* line1 = wavClipOledTitle(clip);

  showOledText(line1, line2);

  if (!wavStreamStart(clip)) {
    showOledText(line1, "Failed");
    return false;
  }

  while (wavStreamUpdate()) {
  }

  showOledText(line1, "OK");
  return true;
}

}  // namespace

bool initAudioStorage() {
  if (audioStorageReady) {
    return true;
  }

  audioStorageReady = LittleFS.begin(
    false,
    "/littlefs",
    10,
    kAudioPartition
  );

  if (!audioStorageReady) {
    Serial.println("LittleFS mount failed (run: pio run -t uploadfs)");
    return false;
  }

  for (uint8_t i = 0; i < kWavClipCount; i++) {
    const WavClip clip = static_cast<WavClip>(i);
    const char* path = wavClipPath(clip);

    if (!LittleFS.exists(path)) {
      Serial.print(path);
      Serial.println(" not on LittleFS");
      logAudioStorageContents();
    }
  }

  return true;
}

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

void stopAllWavPlayback() {
  wavStreamStop();
}

void stopBellPlayback() {
  wavStreamStop();
}

void stopWelcomePlayback() {
  wavStreamStop();
}

void stopAttentionPlayback() {
  wavStreamStop();
}

void stopErrorPlayback() {
  wavStreamStop();
}

void stopAbortPlayback() {
  wavStreamStop();
}

bool startBellPlayback() {
  return wavStreamStart(WavClip::Bell);
}

bool startWelcomePlayback() {
  return wavStreamStart(WavClip::Welcome);
}

bool startAttentionPlayback() {
  return wavStreamStart(WavClip::Attention);
}

bool startErrorPlayback() {
  return wavStreamStart(WavClip::Error);
}

bool startAbortPlayback() {
  return wavStreamStart(WavClip::Abort);
}

bool updateBellPlayback() {
  return wavStreamUpdate();
}

bool updateWelcomePlayback() {
  return wavStreamUpdate();
}

bool updateAttentionPlayback() {
  return wavStreamUpdate();
}

bool updateErrorPlayback() {
  return wavStreamUpdate();
}

bool updateAbortPlayback() {
  return wavStreamUpdate();
}

bool playBell() {
  return playWavBlocking(WavClip::Bell, "Playing...");
}

bool playWelcome() {
  return playWavBlocking(WavClip::Welcome, "Playing...");
}

bool playAttention() {
  return playWavBlocking(WavClip::Attention, "Playing...");
}

bool playError() {
  return playWavBlocking(WavClip::Error, "Playing...");
}

bool playAbort() {
  return playWavBlocking(WavClip::Abort, "Playing...");
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
