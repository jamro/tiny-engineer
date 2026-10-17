#include <Arduino.h>

#include <optional>

#include "animation.h"
#include "api/busy.h"
#include "audio/audio.h"
#include "audio/audio_motion.h"
#include "audio/wav_parser.h"
#include "http/access.h"
#include "http/handlers.h"
#include "http/response.h"
#include "pins.h"
#include "robot_tick.h"
#include "settings/settings.h"
#include "serial_log.h"
#include "sleep.h"

namespace {

constexpr size_t kPlayPrefixLen = sizeof("/play/") - 1;

// One POST /play request. loop() is blocked while the body streams, so the
// session runs tickRobot() between speaker writes.
class PlaySession : public WavStreamParser::Sink {
public:
  void begin(WebServer& server) {
    parser_.reset();
    started_ = false;
    heldBusy_ = false;
    refusal_ = Refusal::check(server, Access::Configured);

    if (refusal_) {
      return;
    }

    if (!apiBusyTryAcquire()) {
      refusal_ = Refusal{503, "busy"};
      return;
    }
    heldBusy_ = true;

    // "/play" or "/play/<name>"; pathArg() would assert on the route without a name.
    const String uri = server.uri();
    const char* name = uri.length() > kPlayPrefixLen ? uri.c_str() + kPlayPrefixLen : "talking";

    // Continuous animations and none play no sound of their own, so they cannot talk over the clip.
    if (!parseAnimationName(name, animation_) ||
        (animation_ != AnimationId::None && !animationIsContinuous(animation_))) {
      refusal_ = Refusal{400, "name must be talking, typing, reading, thinking or none"};
      releaseBusy_();
      return;
    }

    stopAllWavPlayback();
    parser_.emplace(SAMPLE_RATE, *this);
    serialLogPrintln("[play] start");
  }

  void feed(const uint8_t* bytes, size_t length) {
    if (parser_) {
      parser_->feed(bytes, length);
    }
  }

  void end() {
    if (!parser_) {
      releaseBusy_();
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
    releaseBusy_();
  }

  void respond(WebServer& server) {
    if (refusal_) {
      refusal_->send(server);
      return;
    }

    if (parser_->failed()) {
      sendError(server, 415, parser_->error());
      return;
    }

    JsonDocument body;
    body["ok"] = true;
    body["played_ms"] = playedMs();
    sendJson(server, 200, body);
  }

  void onPcm(const int16_t* samples, size_t count) override {
    const uint32_t now = millis();

    if (!started_) {
      // The format is known good by now, so the robot only moves for clips it can play.
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
  std::optional<WavStreamParser> parser_;
  AnimationId animation_ = AnimationId::Talking;
  AnimationId previous_ = AnimationId::None;
  bool started_ = false;
  bool heldBusy_ = false;
  std::optional<Refusal> refusal_;

  void releaseBusy_() {
    if (heldBusy_) {
      apiBusyRelease();
      heldBusy_ = false;
    }
  }

  uint32_t playedMs() const {
    return static_cast<uint32_t>(parser_->samplesDelivered() * 1000ULL / SAMPLE_RATE);
  }
};

PlaySession g_session;

}  // namespace

void handlePlayBody(WebServer& server) {
  HTTPRaw& raw = server.raw();

  switch (raw.status) {
    case RAW_START:
      g_session.begin(server);
      break;
    case RAW_WRITE:
      g_session.feed(raw.buf, raw.currentSize);
      break;
    case RAW_END:
    case RAW_ABORTED:
      g_session.end();
      break;
  }
}

void handlePlayDone(WebServer& server) {
  g_session.respond(server);
}
