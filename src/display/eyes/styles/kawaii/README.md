# Kawaii eye style

Round sparkly eyes, a small mouth, and dithered blush, selected with `eyes_style=kawaii`. User-facing behavior (which mood each animation shows) is documented in [docs/api.md#kawaii-eyes](../../../../../docs/api.md#kawaii-eyes).

Everything in this folder is plain C++ with no Arduino dependencies. It builds in the `native` environment and is tested in [`test/test_kawaii`](../../../../../test/test_kawaii/test_kawaii.cpp). The firmware glue is [`../kawaii.cpp`](../kawaii.cpp).

## Layers

Each frame flows top to bottom:

| File | Owns |
| --- | --- |
| [`../kawaii.cpp`](../kawaii.cpp) | Firmware glue: maps `EyeMode` to a `Mood`, reads `/play` speech, steps the animator, blits the frame to the SSD1306 |
| [`idle.h`](idle.h) | `IdleDirector`: idle personality (Content / Smile with grin, wink, excited, and heart-eye accents) |
| [`speech.h`](speech.h) | `Babbler` and `applyVoice`: syllables and vowel mouth shapes for `talking`, from audio or invented |
| [`expression.h`](expression.h) | `Mood` enum and `expressionFor`: one preset `Expression` per mood |
| [`animator.h`](animator.h) | `Animator`: springs and exponential easing toward the target expression; blink-through for shape swaps |
| [`compose.h`](compose.h) | `composeFace`: merges the classic pose (position, height, blinks) with the eased expression, plus hop and twinkle |
| [`face.h`](face.h) | `render`: draws a `Face` into a 128×32 1-bit buffer. Pure, no state between calls |
| [`util.h`](util.h) | Shared helpers: `clamp01`, `lerp`, wrap-safe `reached`, `RandRange` |

The pose comes from the shared classic eye modes, so glances, scans, blinks, and the `dead` flicker work with no kawaii-specific code.

## Tuning

Every tuning value is a named constant at the top of the file that uses it:

- **Look of a mood**: the preset in `expressionFor` ([`expression.cpp`](expression.cpp)).
- **Shape geometry** (sparkles, happy carve, lid, blush, stars, line shapes): [`face.cpp`](face.cpp).
- **Motion feel** (spring frequency and damping, easing time constants, swap blink): [`animator.cpp`](animator.cpp).
- **Idle timing and accent odds**: the `kAccents` table in [`idle.cpp`](idle.cpp).
- **Talking rhythm and vowel shapes**: the `kVowels` table and babble constants in [`speech.cpp`](speech.cpp).
- **Hop, twinkle, and mouth placement**: [`compose.cpp`](compose.cpp) and `kEyeRadius` / `kMouthDrop` in [`compose.h`](compose.h).

## Adding a mood

1. Add it to `Mood` in [`expression.h`](expression.h). Idle-family moods go before `Love`, because `isIdleMood` relies on that order.
2. Add its name to `kMoodNames` and its preset to `expressionFor`. A `static_assert` and `-Wswitch` catch a missing name or case.
3. Map an `EyeMode` to it in `modeMood` ([`../kawaii.cpp`](../kawaii.cpp)), or add it to the `kAccents` table in [`idle.cpp`](idle.cpp).
4. Add a row to the kawaii table in [docs/api.md](../../../../../docs/api.md#kawaii-eyes).

A new eye shape (`EyeKind`) needs a draw function and a `case` in `drawEye` ([`face.cpp`](face.cpp)). The animator swaps shapes behind a blink on its own.
