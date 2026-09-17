# OLED eye styles

Extensible eye renderers selected by Config / `POST /settings?eyes_style=…`.

| Id | Meaning |
| --- | --- |
| `classic` | Procedural rounded eyes (default) |
| `kaomoji` | Animated faces from TinyEngineerExpressions |

## Add a style

1. Implement an `EyeStyleRenderer` in a new `styles/*.cpp` (export `const EyeStyleRenderer kFooEyeStyle`).
2. Register it in [`registry.cpp`](registry.cpp).
3. Add the string id to [`eye_style_ids.cpp`](eye_style_ids.cpp) (keeps native validate in sync).
4. Add a Config `<select>` option and document `eyes_style` in `docs/api.md` / HTML param table.

The animation registry stays unchanged; styles map `EyeMode` (and sleep phase / `AnimationId::Sleep`) to pixels.
