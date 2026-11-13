# Kawaii eye style

Round eyes with big moving pupils, a small talking mouth, and dithered blush, selected with `eyes_style=kawaii`. User-facing behavior (which mood each animation shows) is documented in [docs/api.md#kawaii-eyes](../../../../../docs/api.md#kawaii-eyes).

Everything in this folder is plain C++ with no Arduino dependencies. It builds in the `native` environment and is tested in [`test/test_kawaii`](../../../../../test/test_kawaii/test_kawaii.cpp). The firmware glue is [`../kawaii.cpp`](../kawaii.cpp).

## Layers

Each frame flows top to bottom:

| File | Owns |
| --- | --- |
| [`../kawaii.cpp`](../kawaii.cpp) | Firmware glue: maps `EyeMode` to a mood (or an activity, the bell, a spoken clip), reads `/play` speech, tracks how long work has gone well, steps the animator, blits the frame to the SSD1306 |
| [`director.h`](director.h) | `MoodDirector`: each activity's personality, a base mood with occasional accents (idle: smiles with grins, winks, excitement, hearts; typing: focused with pleased smiles, grins, winks; reading: pleased reads and "ooh"; thinking: "aha!"). Accents come more often while work goes well. `BellCelebration`: the ring's deep breath, yell, pupil shake (keyed to the stock `bell.wav` loudness), and afterglow |
| [`burst.h`](burst.h) | `addStarBurst`: stars flying out of the eyes on the bell's ding, with trails that fade with the ring |
| [`tracker.h`](tracker.h) | `CodeReader`: the typing gaze (reads a few lines, darts to another spot, repeats) |
| [`lines.h`](lines.h) | Scripted lines keyed to the stock clips (`Line`: welcome, attention, error, abort, dead): the mouth says the actual words in sync. Which clips the face mouths: the `SpokenClip` table in [`../kawaii.cpp`](../kawaii.cpp) |
| [`speech.h`](speech.h) | `Babbler` and `applyVoice`: syllables and vowel mouth shapes for `talking`, from audio or invented |
| [`expression.h`](expression.h) | `Mood` enum, `expressionFor` (one preset `Expression` per mood), and `blend` between two expressions |
| [`animator.h`](animator.h) | `Animator`: springs and exponential easing toward the target expression; blink-through for shape swaps; the pupil shake's phase |
| [`compose.h`](compose.h) | `composeFace`: merges the classic pose (position, height, blinks) with the eased expression, plus hop, breath rise, pupil shake, and twinkle stars |
| [`face.h`](face.h) | `render`: draws a `Face` into a 128×32 1-bit buffer. Pure, no state between calls |
| [`util.h`](util.h) | Shared helpers: `clamp01`, `lerp`, wrap-safe `reached`, `RandRange` |

The pose comes from the shared classic eye modes, so glances, scans, blinks, and the `dead` flicker work with no kawaii-specific code. Two exceptions lead the eyes themselves: typing (its `followGlance` is 0 and `CodeReader` sets the gaze) and the ring (the bell keeps full eye height).

## Tuning

Every tuning value is a named constant at the top of the file that uses it:

- **Look of a mood**: the preset in `expressionFor` ([`expression.cpp`](expression.cpp)).
- **Shape geometry** (pupil and catch-light, happy carve, lid, blush, stars, line shapes): [`face.cpp`](face.cpp).
- **Where the pupils look**: the glance ranges in [`../kawaii.cpp`](../kawaii.cpp) (choreography), plus each mood's `gazeX` / `gazeY` and `pupil` size in [`expression.cpp`](expression.cpp).
- **Motion feel** (spring frequency and damping, easing time constants, swap blink): [`animator.cpp`](animator.cpp).
- **Accent timing and odds per activity**: the `kPersonalities` table in [`director.cpp`](director.cpp); how long work must go well to reach the most frequent accents: `kFlowRampMs` in [`../kawaii.cpp`](../kawaii.cpp).
- **Typing gaze** (line length, word timing, lines per spot, dart range): [`tracker.cpp`](tracker.cpp).
- **Lip sync**: each line's keyframes, its stock cue times (`kScripts`), and `kLeadMs` in [`lines.cpp`](lines.cpp). Re-key a track if its stock clip changes; to mouth another clip, add a `Line`, its track and `kScripts` entry there, and a `SpokenClip` entry in [`../kawaii.cpp`](../kawaii.cpp).
- **The bell celebration**: breath and yell timing, the pupil shake strength and speed, and `kBellRing` (the stock `bell.wav` loudness the shake follows; re-measure it if the clip changes) in [`director.cpp`](director.cpp); the breath, yell, and bliss faces in [`expression.cpp`](expression.cpp); star paths, speed, and trail length in [`burst.cpp`](burst.cpp).
- **Talking rhythm and vowel shapes**: the `kVowels` table and babble constants in [`speech.cpp`](speech.cpp).
- **Hop, twinkle, and mouth placement**: [`compose.cpp`](compose.cpp) and `kEyeRadius` / `kMouthDrop` in [`compose.h`](compose.h).

## Frame budget

Audio clips run through the same `loop()` as the eyes. Each pass feeds one 512-sample WAV chunk (23 ms at 22 050 Hz), then draws a frame every 33 ms. `display.display()` alone holds the I²C bus for about 12 ms. If a pass takes longer than the chunk it delivered, the I2S buffer runs dry and the audio clips, which hits the largest faces first.

The ESP32-C3 has no FPU, so every float operation is a library call. Keep a frame (ease, compose, render) to about **600 k instructions**: the heaviest frame while a clip plays is the bell's yell with its star burst at its busiest (about 550 k), then Excited at the welcome pose (about 420 k); heart eyes take about 1.2 M, but only in idle, with no clip playing.

- Test geometry in float only at the edges of a row, then write whole runs (`runAround`, `LidCut::trim`).
- Precompute per-row and per-column terms instead of recomputing them per pixel.
- Center discs on whole pixels so their rows can be precomputed (`PixelDisc`); that also keeps a moving pupil the same shape.
- Never draw a shape twice per frame.

## Adding a mood

1. Add it to `Mood` in [`expression.h`](expression.h). Idle-family moods go before `Love`, because `isIdleMood` relies on that order.
2. Add its name to `kMoodNames` and its preset to `expressionFor`. A `static_assert` and `-Wswitch` catch a missing name or case.
3. Map an `EyeMode` to it in `modeMood` ([`../kawaii.cpp`](../kawaii.cpp)), or add it as an accent in the `kPersonalities` table in [`director.cpp`](director.cpp).
4. Add a row to the kawaii table in [docs/api.md](../../../../../docs/api.md#kawaii-eyes).

A new eye shape (`EyeKind`) needs a draw function and a `case` in `drawEye` ([`face.cpp`](face.cpp)). The animator swaps shapes behind a blink on its own.
