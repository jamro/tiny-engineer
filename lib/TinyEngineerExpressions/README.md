# Tiny Engineer Expressions

Sixteen animated, kaomoji-inspired faces for a **128 × 32 monochrome OLED**. The C++ library renders a frame into caller-owned memory; the application decides when and where to show it.

This is an optional display library. It does not replace Tiny Engineer's existing eye modes or map expressions to HTTP commands, AI events, or robot poses. It does not access I²C, initialize a display, change its rotation, or issue servo commands.

## Faces

The display numbers below are one-based; the enum values are zero-based and follow this order.

| No. | `Expression` | Face |
| --- | --- | --- |
| 1 | `Idle` | Relaxed idle |
| 2 | `Happy` | Happy |
| 3 | `Laugh` | Laughing |
| 4 | `Wink` | Wink |
| 5 | `Curious` | Curious |
| 6 | `Thinking` | Thinking |
| 7 | `Surprise` | Surprised |
| 8 | `Smug` | Smug |
| 9 | `Sleepy` | Sleepy |
| 10 | `Sleep` | Sleeping |
| 11 | `Sad` | Sad |
| 12 | `Cry` | Crying |
| 13 | `Angry` | Angry |
| 14 | `Panic` | Panicked |
| 15 | `Shy` | Shy |
| 16 | `Love` | Heart eyes |

Each face has 30 frames, spaced 100 ms apart, for a repeating 3-second animation. Faces are original pixel drawings made from geometric primitives, with no external font or image dependency. Source and generated assets use the repository's [MIT license](../../LICENSE).

## Use from C++

```cpp
#include <TinyEngineerExpressions.h>

namespace faces = tiny_engineer::expressions;
uint8_t frame[faces::kFrameBytes]; // 512 bytes, owned by the caller

bool ready = faces::render(faces::Expression::Happy, elapsedMs,
                          frame, sizeof(frame));
```

`render()` uses `elapsedMs` since the expression was selected. Start at zero for the first frame; elapsed time wraps within the animation every `kLoopDurationMs` (3000 ms). The function has no clock or playback state and does not delay. The caller controls updates, expression selection, and stopping. For Arduino, unsigned `millis() - startedAt` subtraction handles a timer rollover; reset or advance the start timestamp during long-running playback, as the example does.

`render()` returns `false` for an invalid expression, a null buffer, or a buffer smaller than `kFrameBytes`, leaving the supplied buffer unchanged. `name()` returns a stable lowercase identifier for a valid expression and `nullptr` for an invalid one. There is no dynamic allocation in the renderer.

The result is **row-major, MSB-first**, 16 bytes per row: pixel `(0, 0)` is bit 7 of byte 0. It is compatible with Adafruit GFX `drawBitmap()`. It is **not** the vertical-page layout returned by SSD1306 `getBuffer()`; do not copy it directly there.

For an already initialized Adafruit display:

```cpp
if (faces::render(selectedExpression, elapsedMs, frame, sizeof(frame))) {
    display.clearDisplay();
    display.drawBitmap(0, 0, frame, faces::kWidth, faces::kHeight,
                       SSD1306_WHITE);
    display.display();
}
```

The application still owns display initialization, clearing, brightness, orientation, and sleep behavior. When integrating into the robot firmware, use the existing display owner and saved rotation setting. Avoid two renderers updating the same display concurrently. Selecting this library must be an explicit application decision, so the existing eye-mode and motion behavior can remain the default.

## Standalone OLED demo

[`examples/expression-demo/main.cpp`](../../examples/expression-demo/main.cpp) cycles through all 16 faces, showing each for three seconds. It schedules frames without `delay()` and uses a static 512-byte decode buffer. Adafruit SSD1306 additionally owns its normal display framebuffer.

The example targets the Waveshare ESP32-C3-Zero with SDA = GPIO 0, SCL = GPIO 1, and OLED address `0x3C`. Rotation defaults to 0; add `-D EXPRESSION_DEMO_ROTATION=2` to the example environment's `build_flags` for a display mounted upside down. This affects only the example, not the robot's saved settings.

Build only:

```bash
pio run -e expression-demo
```

For an intentional bench test, keep the **separate servo V+ rail switched off**, connect the OLED and ESP32 via their normal logic wiring, and power the ESP32 through USB. This example does not send PCA9685 commands, so it does not disable outputs that a powered controller might retain. Uploading the demo replaces the application currently on the ESP32; retain your previous firmware if you want to restore it.

```bash
pio run -e expression-demo -t upload
pio device monitor -e expression-demo
```

Observe all 16 expressions, a complete 48-second cycle, the screen orientation, and smooth frame updates. Hardware behavior must be checked on the target display before calling the library bench-tested.

## Assets and checks

The deterministic generator packs the pixels, deduplicates identical frames, and applies lossless byte-run encoding. The 480 frame references share 87 unique frames; the encoded payload and lookup tables occupy about 18.2 KiB. Decoding requires the caller's 512-byte output buffer rather than a table of uncompressed animation frames.

From the repository root:

```bash
node scripts/expressions/generate.js --check
node scripts/expressions/test-assets.js
pio test -e native
pio run
pio run -e expression-demo
```

After editing the drawing source, regenerate assets with `node scripts/expressions/generate.js` and review the resulting pixel changes. The checks cover generated-file consistency, frame data, and the C++ renderer. Compilation and host checks do not establish I²C reliability, OLED orientation, or on-device animation quality; record those results separately in a PR.
