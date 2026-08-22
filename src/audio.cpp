#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <LittleFS.h>
#include <math.h>

#include "pins.h"
#include "oled.h"
#include "audio.h"

I2SClass I2S;

namespace {

bool audioStorageReady = false;

constexpr const char* kAudioPartition = "spiffs";
constexpr const char* kBellPath = "/bell.wav";

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

bool skipWavPcmData(File& file) {
  char tag[4];

  if (file.readBytes(tag, 4) != 4 ||
      memcmp(tag, "RIFF", 4) != 0) {
    return false;
  }

  file.seek(8);

  if (file.readBytes(tag, 4) != 4 ||
      memcmp(tag, "WAVE", 4) != 0) {
    return false;
  }

  while (file.available()) {
    if (file.readBytes(tag, 4) != 4) {
      return false;
    }

    uint32_t chunkSize = 0;

    if (file.read(
          reinterpret_cast<uint8_t*>(&chunkSize),
          sizeof(chunkSize)
        ) != sizeof(chunkSize)) {
      return false;
    }

    if (memcmp(tag, "data", 4) == 0) {
      return true;
    }

    if (!file.seek(file.position() + chunkSize)) {
      return false;
    }
  }

  return false;
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

  if (!LittleFS.exists(kBellPath)) {
    Serial.println("bell.wav not on LittleFS");
    logAudioStorageContents();
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

bool playBell() {
  constexpr int FRAMES = 512;

  int16_t buffer[FRAMES * 2];
  uint8_t pcmBytes[FRAMES * 2];

  showOledText(
    "BELL",
    "Playing..."
  );

  Serial.println("Playing bell.wav");

  if (!initAudioStorage()) {
    showOledText(
      "BELL",
      "FS error"
    );
    Serial.println("Bell failed: filesystem");
    return false;
  }

  File file = LittleFS.open(kBellPath, "r");

  if (!file) {
    logAudioStorageContents();
    showOledText(
      "BELL",
      "Missing file"
    );
    Serial.println("Bell failed: bell.wav missing");
    return false;
  }

  if (!skipWavPcmData(file)) {
    file.close();
    showOledText(
      "BELL",
      "Bad WAV"
    );
    Serial.println("Bell failed: invalid WAV");
    return false;
  }

  while (file.available()) {
    const size_t bytesRead =
      file.read(
        pcmBytes,
        sizeof(pcmBytes)
      );

    if (bytesRead < 2) {
      break;
    }

    const int framesThisTime =
      (int)(bytesRead / 2);

    for (int i = 0; i < framesThisTime; i++) {
      const int16_t sample =
        (int16_t)(
          pcmBytes[i * 2] |
          (pcmBytes[i * 2 + 1] << 8)
        );

      buffer[i * 2] = sample;
      buffer[i * 2 + 1] = sample;
    }

    I2S.write(
      (uint8_t*)buffer,
      framesThisTime *
      2 *
      sizeof(int16_t)
    );
  }

  file.close();

  showOledText(
    "BELL",
    "OK"
  );

  Serial.println("Bell OK");
  return true;
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
