#include "display/eyes/styles/kawaii/tracker.h"

namespace kawaii {

namespace {

constexpr float kPercent = 100.0f;

// Where in the "file" a spot can be, in gaze units.
constexpr int32_t kSpotMinXPct = -55;
constexpr int32_t kSpotMaxXPct = 25;   // lines start left and run right
constexpr int32_t kSpotMinYPct = -35;
constexpr int32_t kSpotMaxYPct = 40;
constexpr float kLineLength = 0.45f;   // gaze travelled along one line
constexpr float kLineSpacing = 0.12f;  // down to the next line
constexpr float kLowestLine = 0.8f;

constexpr uint32_t kLinesMin = 2;      // read at one spot before darting
constexpr uint32_t kLinesMax = 4;
constexpr uint32_t kWordStepMinPct = 7;
constexpr uint32_t kWordStepMaxPct = 14;
constexpr uint32_t kWordMinMs = 180;
constexpr uint32_t kWordMaxMs = 420;
constexpr uint32_t kLineReturnMinMs = 200;  // settle after the sweep back
constexpr uint32_t kLineReturnMaxMs = 350;
constexpr uint32_t kFixateMinMs = 300;      // after a dart
constexpr uint32_t kFixateMaxMs = 650;

}  // namespace

uint32_t CodeReader::between(uint32_t lo, uint32_t hi) {
  return rand_(lo, hi + 1);
}

void CodeReader::reset(uint32_t now) {
  armed_ = true;
  dart(now);
}

void CodeReader::dart(uint32_t now) {
  const uint32_t xSpan = (uint32_t)(kSpotMaxXPct - kSpotMinXPct);
  const uint32_t ySpan = (uint32_t)(kSpotMaxYPct - kSpotMinYPct);
  lineStart_ = (float)(kSpotMinXPct + (int32_t)between(0, xSpan)) / kPercent;
  x_ = lineStart_;
  y_ = (float)(kSpotMinYPct + (int32_t)between(0, ySpan)) / kPercent;
  linesLeft_ = between(kLinesMin, kLinesMax);
  nextAt_ = now + between(kFixateMinMs, kFixateMaxMs);
}

void CodeReader::nextLine(uint32_t now) {
  linesLeft_--;
  x_ = lineStart_;
  y_ = y_ + kLineSpacing > kLowestLine ? kLowestLine : y_ + kLineSpacing;
  nextAt_ = now + between(kLineReturnMinMs, kLineReturnMaxMs);
}

void CodeReader::nextWord(uint32_t now) {
  x_ += (float)between(kWordStepMinPct, kWordStepMaxPct) / kPercent;
  nextAt_ = now + between(kWordMinMs, kWordMaxMs);
}

Gaze CodeReader::update(uint32_t now) {
  if (!armed_) {
    reset(now);
  }
  if (reached(now, nextAt_)) {
    if (x_ < lineStart_ + kLineLength) {
      nextWord(now);
    } else if (linesLeft_ > 1) {
      nextLine(now);
    } else {
      dart(now);
    }
  }
  return {x_, y_};
}

}  // namespace kawaii
