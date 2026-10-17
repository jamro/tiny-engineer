#include "serial/play.h"

#include <Arduino.h>
#include <optional>

#include "animation.h"
#include "api/busy.h"
#include "audio/audio.h"
#include "audio/audio_motion.h"
#include "audio/wav_parser.h"
#include "pins.h"
#include "robot_tick.h"
#include "serial_log.h"
#include "sleep.h"

namespace {

class SerialPlaySession : public WavStreamParser::Sink {
public:
  bool begin(const char* name, const char*& error) {
    if (active_) {
      error = "busy";
      return false;
    }
    if (!apiBusyTryAcquire()) {
      error = "busy";
      return false;
    }
    heldBusy_ = true;

    if (name == nullptr) {
      name = "talking";
    }
    if (!parseAnimationName(name, animation_) ||
        (animation_ != AnimationId::None && !animationIsContinuous(animation_))) {
      error = "name must be talking, typing, reading, thinking or none";
      release_();
      return false;
    }

    stopAllWavPlayback();
    parser_.emplace(SAMPLE_RATE, *this);
    streamId_ = nextStreamId_++;
    if (nextStreamId_ == 0) {
      nextStreamId_ = 1;
    }
    // One credit window: host waits for a credit grant before the next chunk.
    credits_ = static_cast<uint16_t>(kSerialPlayCreditChunk);
    started_ = false;
    active_ = true;
    failed_ = false;
    serialLogPrintln("[play] serial start");
    return true;
  }

  bool feed(
    uint8_t streamId,
    const uint8_t* data,
    size_t length,
    uint16_t& creditsOut,
    const char*& error
  ) {
    if (!active_ || streamId != streamId_ || !parser_) {
      error = "no play session";
      return false;
    }
    if (length == 0 || length > credits_) {
      error = "over credits";
      creditsOut = 0;
      abort_();
      return false;
    }
    credits_ = 0;
    parser_->feed(data, length);
    if (parser_->failed()) {
      error = parser_->error() != nullptr ? parser_->error() : "wav parse failed";
      creditsOut = 0;
      abort_();
      return false;
    }
    // writeMonoToSpeaker blocks inside onPcm, so the chunk is drained before we grant again.
    credits_ = static_cast<uint16_t>(kSerialPlayCreditChunk);
    creditsOut = credits_;
    return true;
  }

  bool end(uint8_t streamId, uint32_t& playedMs, const char*& error) {
    if (!active_ || streamId != streamId_) {
      error = "no play session";
      return false;
    }
    if (parser_) {
      parser_->finish();
      if (parser_->failed()) {
        error = parser_->error();
        failed_ = true;
      } else {
        playedMs = static_cast<uint32_t>(parser_->samplesDelivered() * 1000ULL / SAMPLE_RATE);
      }
    }
    serialLogPrint("[play] serial end ms=");
    serialLogPrintln(playedMs);

    restoreAnimIfStarted_();
    release_();
    if (failed_) {
      return false;
    }
    return true;
  }

  uint8_t streamId() const { return streamId_; }
  uint16_t credits() const { return credits_; }
  bool active() const { return active_; }

  void onPcm(const int16_t* samples, size_t count) override {
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
    tickRobot();
  }

private:
  void restoreAnimIfStarted_() {
    if (!started_) {
      return;
    }
    if (animation_ == AnimationId::Talking) {
      audioMotionEnd(millis());
    }
    setAnimationImmediately(animationIsContinuous(previous_) ? previous_ : AnimationId::None);
    started_ = false;
  }

  // End session after a rejected DATA chunk: free busy so later play is not stuck at 503.
  void abort_() {
    restoreAnimIfStarted_();
    release_();
  }

  void release_() {
    parser_.reset();
    active_ = false;
    if (heldBusy_) {
      apiBusyRelease();
      heldBusy_ = false;
    }
  }

  std::optional<WavStreamParser> parser_;
  AnimationId animation_ = AnimationId::Talking;
  AnimationId previous_ = AnimationId::None;
  uint8_t streamId_ = 0;
  uint16_t credits_ = 0;
  bool started_ = false;
  bool active_ = false;
  bool heldBusy_ = false;
  bool failed_ = false;
  const char* error_ = nullptr;
  uint8_t nextStreamId_ = 1;
};

SerialPlaySession g_play;

}  // namespace

bool serialPlayBegin(const char* animName, uint8_t& streamId, uint16_t& credits, const char*& error) {
  if (!g_play.begin(animName, error)) {
    return false;
  }
  streamId = g_play.streamId();
  credits = g_play.credits();
  return true;
}

bool serialPlayFeed(
  uint8_t streamId,
  const uint8_t* data,
  size_t length,
  uint16_t& creditsOut,
  const char*& error
) {
  return g_play.feed(streamId, data, length, creditsOut, error);
}

bool serialPlayEnd(uint8_t streamId, uint32_t& playedMs, const char*& error) {
  playedMs = 0;
  return g_play.end(streamId, playedMs, error);
}

bool serialPlayActive() {
  return g_play.active();
}
