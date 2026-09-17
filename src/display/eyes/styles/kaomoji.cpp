#include "display/eyes/styles/eye_style.h"
#include "display/eyes/styles/kaomoji.h"

#include <Adafruit_SSD1306.h>
#include <TinyEngineerExpressions.h>

#include "display/oled.h"
#include "display/oled_internal.h"

namespace {

namespace expressions = tiny_engineer::expressions;

uint8_t g_kaomojiFrame[expressions::kFrameBytes];
expressions::Expression g_activeExpression = expressions::Expression::Idle;
uint32_t g_expressionStartedMs = 0;
bool g_expressionArmed = false;

expressions::Expression expressionFor(
  EyeMode mode,
  AnimationId animation,
  EyeStyleSleepPhase sleepPhase
) {
  if (sleepPhase == EyeStyleSleepPhase::Closing ||
      animation == AnimationId::Sleep) {
    return expressions::Expression::Sleep;
  }

  if (sleepPhase == EyeStyleSleepPhase::Opening) {
    return expressions::Expression::Idle;
  }

  switch (mode) {
    case EyeMode::Idle:
      return expressions::Expression::Idle;
    case EyeMode::Typing:
    case EyeMode::Thinking:
      return expressions::Expression::Thinking;
    case EyeMode::Reading:
    case EyeMode::Attention:
      return expressions::Expression::Curious;
    case EyeMode::Ring:
      return expressions::Expression::Surprise;
    case EyeMode::Welcome:
      return expressions::Expression::Happy;
    case EyeMode::Error:
      return expressions::Expression::Angry;
    case EyeMode::Abort:
      return expressions::Expression::Panic;
    case EyeMode::Wakeup:
      return expressions::Expression::Sleepy;
    case EyeMode::Dead:
      return expressions::Expression::Sad;
  }

  return expressions::Expression::Idle;
}

void drawKaomoji(
  EyeMode mode,
  AnimationId animation,
  uint32_t /*modeStartedMs*/,
  uint32_t now,
  EyeStyleSleepPhase sleepPhase
) {
  if (!oledAvailable) {
    return;
  }

  const auto expression = expressionFor(mode, animation, sleepPhase);
  if (!g_expressionArmed || expression != g_activeExpression) {
    g_activeExpression = expression;
    g_expressionStartedMs = now;
    g_expressionArmed = true;
  }

  const uint32_t elapsed = now - g_expressionStartedMs;

  if (!expressions::render(
        expression,
        elapsed,
        g_kaomojiFrame,
        sizeof(g_kaomojiFrame)
      )) {
    return;
  }

  display.clearDisplay();
  display.drawBitmap(
    0,
    0,
    g_kaomojiFrame,
    expressions::kWidth,
    expressions::kHeight,
    SSD1306_WHITE
  );
  display.display();
}

}  // namespace

void kaomojiResetPlayback(uint32_t now) {
  g_expressionArmed = false;
  g_expressionStartedMs = now;
  g_activeExpression = expressions::Expression::Idle;
}

extern const EyeStyleRenderer kKaomojiEyeStyle = {
  "kaomoji",
  false,
  false,
  drawKaomoji,
};
