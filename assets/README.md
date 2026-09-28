# Assets

Source files for Tiny Engineer branding and speaker clips. Firmware does not read this folder at runtime.

## Audio

WAV clips played by animations and the setup wizard. Format: **44100 Hz, mono, 16-bit PCM**.

`pio run` copies these files into `data/` via [`scripts/copy_assets.py`](../scripts/copy_assets.py). The next filesystem image overwrites `data/`, so edit the files here. Flash them with `pio run -t upload` or `pio run -t uploadfs`.

| File | Duration | Transcript |
| --- | --- | --- |
| [`abort.wav`](abort.wav) | 2.5 s | Fine! I didn't want to finish that anyway! |
| [`attention.wav`](attention.wav) | 3.0 s | Psst, Human! I might want to take a look. |
| [`bell.wav`](bell.wav) | 2.0 s | Bell sound, no voiceover. |
| [`dead.wav`](dead.wav) | 3.4 s | Insufficient resources, shutting dooooowwwwwnnnn.... |
| [`error.wav`](error.wav) | 2.2 s | O-oh, human! We have a problem! |
| [`welcome.wav`](welcome.wav) | 2.7 s | Hello human! What are we building today? |

Animation timing is synced to these clips. After replacing a file, check the matching pose in `src/animation/`. Playback details: [docs/api.md](../docs/api.md).

## Logo

[`logo/tiny_engineer_logo.svg`](logo/tiny_engineer_logo.svg) — wordmark (Inkscape, 180×80 mm). Branding only; not copied to LittleFS.
