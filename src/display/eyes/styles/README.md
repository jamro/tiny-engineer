# OLED eye styles

Extensible eye renderers selected by Config / `POST /settings` `{"eyes_style":"…"}`.

| Id | Meaning |
| --- | --- |
| `classic` | Procedural rounded eyes (default) |
| `kaomoji` | Animated faces from TinyEngineerExpressions |
| `cover` | Full-half bars for mods with a mask with eye holes; height changes via top and/or bottom black margins |
| `dots` | 12×12 filled circles; diameter tracks pose height, center tracks pose |
| `kawaii` | Round eyes with big moving pupils, a talking mouth, and blush. Each animation has its own mood (with occasional accents while idle, typing, reading, or thinking), the mouth says the stock voice clips in sync, and the bell gets a celebration ([`kawaii/`](kawaii/)) |
| `evil` | Solid white scowling silhouettes (pointed high outers, rounded low inners, concave brow notch, convex bottoms; no pupils). Always menacing; pose and blink track classic |

## Add a style

1. Implement an `EyeStyleRenderer` in a new `styles/*.cpp` (export `const EyeStyleRenderer kFooEyeStyle`).
2. Register it in [`registry.cpp`](registry.cpp).
3. Add the string id to [`eye_style_ids.cpp`](eye_style_ids.cpp) (keeps native validate in sync).
4. Add a Config `<select>` option and document `eyes_style` in `docs/api.md` / HTML param table.

The animation registry stays unchanged; styles map `EyeMode` (and sleep phase / `AnimationId::Sleep`) to pixels.

## Keep audio fed (I2S underrun)

Welcome / attention / error / abort / dead pump LittleFS WAV audio **in the same main-loop tick** as the OLED redraw (`updateAnimation` → `updateEyes` → `style->draw`). The I2S DMA buffer is only tens of milliseconds deep.

A slow `draw` (for example thousands of `drawPixel` calls with per-pixel float math every ~33 ms) starves `wavStreamUpdate`: speech sounds **slow or gappy**, and wall-clock cue timers (`WELCOME_AUDIO_END_MS`, …) **cut the clip early**.

**Do:**

- Prefer `fillRect` / `drawFastVLine` / `drawFastHLine` / small bitmasks (see `classic`, `cover`, `dots`, `evil`)
- Bound work to the eye boxes; avoid full-frame nested loops
- On device, play `welcome` (or another stock clip) with the new style and listen for normal speed and a full ending

**Do not** rely on host-only preview timing; the robot tick is what matters.
