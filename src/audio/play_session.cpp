#include "audio/play_session.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

#include "audio/audio.h"
#include "audio/audio_motion.h"
#include "pins.h"
#include "robot_tick.h"
#include "settings/settings.h"
#include "serial_log.h"
#include "sleep.h"

const char* playAnimNameFromPath(const char* path) {
  if (path == nullptr) {
    return nullptr;
  }
  if (std::strcmp(path, "/play") == 0) {
    return "talking";
  }
  static constexpr char kPrefix[] = "/play/";
  static constexpr size_t kPrefixLen = sizeof(kPrefix) - 1;
  if (std::strncmp(path, kPrefix, kPrefixLen) == 0 && path[kPrefixLen] != '\0') {
    return path + kPrefixLen;
  }
  return nullptr;
}

bool isPlayApiPath(const char* path) {
  return playAnimNameFromPath(path) != nullptr;
}

void PlaySession::formatError(const char* message) {
  snprintf(body_, sizeof(body_), "{\"ok\":false,\"error\":\"%s\"}", message);
}

void PlaySession::clear() {
  parser_.reset();
  animation_ = AnimationId::Talking;
  previous_ = AnimationId::None;
  started_ = false;
  ended_ = false;
  rejectCode_ = 0;
  lastTickMs_ = 0;
  body_[0] = '\0';
}

void PlaySession::reject(int code, const char* message) {
  rejectCode_ = code;
  formatError(message);
  parser_.reset();
}

void PlaySession::begin(const char* animName) {
  clear();

  if (!settingsWifiConfigured()) {
    reject(503, "wifi not configured");
    return;
  }

  const char* name = (animName != nullptr && animName[0] != '\0') ? animName : "talking";

  if (!parseAnimationName(name, animation_) ||
      (animation_ != AnimationId::None && !animationIsContinuous(animation_))) {
    reject(400, "name must be talking, typing, reading, thinking or none");
    return;
  }

  stopAllWavPlayback();
  parser_.emplace(SAMPLE_RATE, *this);
  serialLogPrintln("[play] start");
}

void PlaySession::feed(const uint8_t* bytes, size_t length) {
  if (parser_ && length > 0) {
    parser_->feed(bytes, length);
  }
}

void PlaySession::end() {
  if (ended_) {
    return;
  }
  ended_ = true;

  if (!parser_) {
    return;
  }

  parser_->finish();
  serialLogPrint("[play] end ms=");
  serialLogPrintln(playedMs());

  if (started_) {
    if (animation_ == AnimationId::Talking) {
      audioMotionEnd(millis());
    }
    setAnimationImmediately(animationIsContinuous(previous_) ? previous_ : AnimationId::None);
  }

  if (rejectCode_ != 0) {
    return;
  }

  if (parser_->failed()) {
    formatError(parser_->error());
    rejectCode_ = 415;
    return;
  }

  snprintf(body_, sizeof(body_), "{\"ok\":true,\"played_ms\":%lu}",
           static_cast<unsigned long>(playedMs()));
}

int PlaySession::statusCode() const {
  if (rejectCode_ != 0) {
    return rejectCode_;
  }
  return 200;
}

uint32_t PlaySession::playedMs() const {
  if (!parser_) {
    return 0;
  }
  return static_cast<uint32_t>(parser_->samplesDelivered() * 1000ULL / SAMPLE_RATE);
}

void PlaySession::onPcm(const int16_t* samples, size_t count) {
  const uint32_t now = millis();

  if (!started_) {
    started_ = true;
    previous_ = getAnimation();
    if (animation_ == AnimationId::Talking) {
      audioMotionBegin(now);
    }
    setAnimationImmediately(animation_);
  }

  if (animation_ == AnimationId::Talking) {
    audioMotionFeed(samples, count, now);
  }

  writeMonoToSpeaker(samples, count);
  noteActivity(now);
  // Eyes/servos ~50 Hz is enough; skipping tick every PCM batch frees time to
  // drain USB CDC during serial /play (avoids RX overflow → crackle then mute).
  if (lastTickMs_ == 0 || now - lastTickMs_ >= 20) {
    lastTickMs_ = now;
    tickRobot();
  }
}
