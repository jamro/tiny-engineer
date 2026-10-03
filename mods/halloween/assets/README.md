# Assets

Source files for Tiny Engineer branding and speaker clips. Firmware does not read this folder at runtime.

## Audio

WAV clips played by animations and the setup wizard. Format: **44100 Hz, mono, 16-bit PCM**. The build downsamples them to 22050 Hz, the firmware's playback rate, when it packs LittleFS.

These files are not in the default image. Set `custom_audio_mod = halloween` in [`platformio.ini`](../../../platformio.ini). [`scripts/copy_assets.py`](../../../scripts/copy_assets.py) copies stock [`assets/`](../../../assets/) first, then overwrites any clip that exists here. `bell` and `dead` are absent, so those stay stock. Flash with `pio run -t upload` or `pio run -t uploadfs`. Do not edit `data/` by hand.

Each replacement ships a `.cue` of phrase marks (`key=ms`). Firmware reads `/welcome.cue` and the other names from LittleFS and uses them instead of the stock timings. A later cue edit is an `uploadfs` only. Marks below were taken from speech energy in these WAVs; nudge the numbers if a gesture lands on the wrong word.

| File | Duration | Transcript |
| --- | --- | --- |
| [`abort.wav`](abort.wav) | 3.2 s | Cancelled! Great. I’ll just sit here and rot. |
| [`attention.wav`](attention.wav) | 3.3 s | Psst, human! I need your mortal wisdom. |
| [`error.wav`](error.wav) | 2.8 s | Uh-oh! I think our code is haunted. |
| [`welcome.wav`](welcome.wav) | 4.7 s | Hello, human! What are we carving… I mean, building today? |

Phrase timing is the matching `.cue` (`abort.cue`, `attention.cue`, `error.cue`, `welcome.cue`), not the stock constants in `src/animation/`. Playback details: [docs/api.md](../../../docs/api.md).

## Logo

[`logo/tiny_engineer_logo.svg`](../../../assets/logo/tiny_engineer_logo.svg) — wordmark (Inkscape, 180×80 mm). Branding only; not copied to LittleFS.
