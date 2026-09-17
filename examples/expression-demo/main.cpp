#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyEngineerExpressions.h>

// Adafruit rotations: 0 = normal, 2 = 180 degrees. Match your mounting.
#ifndef EXPRESSION_DEMO_ROTATION
#define EXPRESSION_DEMO_ROTATION 0
#endif

namespace {
namespace expressions = tiny_engineer::expressions;

constexpr uint8_t kRotation = EXPRESSION_DEMO_ROTATION;
static_assert(kRotation == 0 || kRotation == 2,
              "The 128 x 32 demo supports rotation 0 or 2");
constexpr uint8_t kOledAddress = 0x3C;
constexpr int kSda = 0;
constexpr int kScl = 1;

Adafruit_SSD1306 display(expressions::kWidth, expressions::kHeight, &Wire, -1);
uint8_t frame[expressions::kFrameBytes];
uint8_t expressionIndex = 0;
uint32_t expressionStarted = 0;
uint32_t lastRendered = 0;
bool displayReady = false;
}

void setup() {
    // This example does not initialize a servo controller. Keep the separate
    // servo V+ supply off when testing an OLED on an assembled robot.
    Serial.begin(115200);
    Wire.begin(kSda, kScl);
    Wire.beginTransmission(kOledAddress);
    if (Wire.endTransmission() != 0) {
        Serial.println("No OLED response at I2C address 0x3C");
        return;
    }
    displayReady = display.begin(SSD1306_SWITCHCAPVCC, kOledAddress, true, false);
    if (!displayReady) {
        Serial.println("OLED initialization failed");
        return;
    }
    display.setRotation(kRotation);
    expressionStarted = millis();
    lastRendered = expressionStarted - expressions::kFrameDurationMs;
    Serial.println("Expression: idle");
}

void loop() {
    if (!displayReady) return;

    const uint32_t now = millis();
    if (uint32_t(now - lastRendered) < expressions::kFrameDurationMs) return;
    lastRendered = now;

    uint32_t elapsed = now - expressionStarted;
    if (elapsed >= expressions::kLoopDurationMs) {
        const uint32_t skipped = elapsed / expressions::kLoopDurationMs;
        expressionIndex = (expressionIndex + skipped) % expressions::kExpressionCount;
        elapsed %= expressions::kLoopDurationMs;
        expressionStarted = now - elapsed;
        Serial.print("Expression: ");
        Serial.println(expressions::name(static_cast<expressions::Expression>(expressionIndex)));
    }

    const auto expression = static_cast<expressions::Expression>(expressionIndex);
    if (expressions::render(expression, elapsed, frame, sizeof(frame))) {
        display.clearDisplay();
        display.drawBitmap(0, 0, frame, expressions::kWidth,
                           expressions::kHeight, SSD1306_WHITE);
        display.display();
    }
}
