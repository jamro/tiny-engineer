#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "animation.h"
#include "audio/wav_parser.h"

// Streaming POST /play session shared by HTTP and serial.
// Call begin → feed (zero or more) → end → read statusCode/bodyJson.
class PlaySession : public WavStreamParser::Sink {
public:
  void clear();

  // animName: talking|typing|reading|thinking|none (default talking if null/empty).
  // Checks Wi-Fi credentials; does not check Bearer (HTTP does that separately).
  void begin(const char* animName);

  void reject(int code, const char* message);

  void feed(const uint8_t* bytes, size_t length);
  void end();

  bool rejected() const { return rejectCode_ != 0; }
  // After end(): HTTP/serial status (200 / 415 / earlier reject).
  int statusCode() const;
  // JSON object string for the reply body.
  const char* bodyJson() const { return body_; }

  void onPcm(const int16_t* samples, size_t count) override;

private:
  std::optional<WavStreamParser> parser_;
  AnimationId animation_ = AnimationId::Talking;
  AnimationId previous_ = AnimationId::None;
  bool started_ = false;
  bool ended_ = false;
  int rejectCode_ = 0;
  uint32_t lastTickMs_ = 0;
  char body_[96] = {};

  void formatError(const char* message);
  uint32_t playedMs() const;
};

// "/play" → "talking"; "/play/foo" → "foo"; else nullptr.
const char* playAnimNameFromPath(const char* path);
bool isPlayApiPath(const char* path);
