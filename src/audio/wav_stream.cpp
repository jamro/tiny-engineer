#include "audio/wav_stream.h"

#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <LittleFS.h>

#include "audio.h"

namespace {

constexpr int kAudioFrames = 512;

struct WavClipDef {
  const char* path;
  const char* label;
  const char* oledTitle;
};

constexpr WavClipDef kClips[kWavClipCount] = {
  {"/bell.wav", "Bell", "BELL"},
  {"/welcome.wav", "Welcome", "WELCOME"},
  {"/attention.wav", "Attention", "ATTENTION"},
  {"/error.wav", "Error", "ERROR"},
};

File g_file;
bool g_playing = false;
WavClip g_active = WavClip::Bell;

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

bool pumpWavChunk(File& file) {
  int16_t buffer[kAudioFrames * 2];
  uint8_t pcmBytes[kAudioFrames * 2];

  const size_t bytesRead =
    file.read(
      pcmBytes,
      sizeof(pcmBytes)
    );

  if (bytesRead < 2) {
    return false;
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

  return file.available() > 0;
}

const WavClipDef& clipDef(WavClip clip) {
  return kClips[static_cast<uint8_t>(clip)];
}

}  // namespace

const char* wavClipPath(WavClip clip) {
  return clipDef(clip).path;
}

const char* wavClipOledTitle(WavClip clip) {
  return clipDef(clip).oledTitle;
}

bool wavStreamStart(WavClip clip) {
  wavStreamStop();

  const WavClipDef& def = clipDef(clip);

  if (!initAudioStorage()) {
    Serial.print(def.label);
    Serial.println(" failed: filesystem");
    return false;
  }

  g_file = LittleFS.open(def.path, "r");

  if (!g_file) {
    logAudioStorageContents();
    Serial.print(def.label);
    Serial.print(" failed: ");
    Serial.print(def.path);
    Serial.println(" missing");
    return false;
  }

  if (!skipWavPcmData(g_file)) {
    g_file.close();
    Serial.print(def.label);
    Serial.println(" failed: invalid WAV");
    return false;
  }

  g_active = clip;
  g_playing = true;
  Serial.print("Playing ");
  Serial.println(def.path);
  return true;
}

bool wavStreamUpdate() {
  if (!g_playing) {
    return false;
  }

  const WavClipDef& def = clipDef(g_active);

  if (!pumpWavChunk(g_file)) {
    if (g_file) {
      g_file.close();
    }
    g_playing = false;
    Serial.print(def.label);
    Serial.println(" OK");
    return false;
  }

  return true;
}

void wavStreamStop() {
  if (g_file) {
    g_file.close();
  }

  g_playing = false;
}

bool wavStreamIsPlaying(WavClip clip) {
  return g_playing && g_active == clip;
}
