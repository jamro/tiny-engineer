# Mods

Optional add-ons for Tiny Engineer. Stock robot parts live in [`3d_models/`](../3d_models/).

A mod can hold models and anything else (notes, images, firmware snippets). Printable geometry and Fusion source stay under that mod’s `3d_models/` folder.

## Layout

```text
mods/<mod_name>/
  README.md              # optional: what it is, non-model notes
  3d_models/
    cad/                 # Fusion source, when the mod has one
    parts/{servo_id}/    # exports; same idea as 3d_models/parts/
  ...                    # anything that is not a model
```

`parts/{servo_id}/` mirrors stock: `3mf/`, `stl/`, and `step/`. Example: [`halloween/`](halloween/).

## Audio

Optional speaker replacements live in `mods/<mod_name>/assets/`. Format matches stock: **44100 Hz, mono, 16-bit PCM**; the build downsamples to 22050 Hz. Firmware still plays `/welcome.wav` and the other root names. The build overlay copies stock [`assets/`](../assets/) first, then overwrites any basename the mod ships. A missing file stays the stock clip.

Stock image (no mod): **[Web flash](https://jamro.github.io/tiny-engineer/flash/)**. Audio mods need PlatformIO: set `custom_audio_mod = halloween` in [`platformio.ini`](../platformio.ini), then `pio run -t upload`. Empty option keeps stock. Details: [docs/flash.md](../docs/flash.md) (Advanced: PlatformIO).

Clips with phrase marks (`welcome`, `attention`, `error`, `abort`, `dead`) need a sibling `.cue` next to the replacement WAV. `bell` has no marks. A cue without its WAV is rejected. Lines are `key=ms` (`#` comments allowed). `end_ms` must be within 200 ms of the WAV duration, phases must increase, and a blink must sit inside its window. The pack step checks this before the image is built.

Firmware loads `/welcome.cue` (and the other clip names) from LittleFS at boot. A missing or invalid file keeps the stock marks compiled into the firmware. Editing a `.cue` and running `pio run -t uploadfs` changes timing without a gesture-code change.

| Clip | Keys, in phase order |
| --- | --- |
| `welcome` | `greeting_end_ms`, `pause_end_ms`, `question_end_ms`, `end_ms`. Blink: `blink_start_ms`, `blink_end_ms` inside greeting→pause. |
| `attention` | `pst_end_ms`, `human_end_ms`, `end_ms`. Blink inside pst→human. |
| `error` | `uhoh_end_ms`, `human_end_ms`, `problem_end_ms`, `end_ms` |
| `abort` | `fine_end_ms`, `didnt_want_end_ms`, `finish_end_ms`, `end_ms` |
| `dead` | `dense_ms`, `shutdown_ms`, `end_ms` |

## Workflow

Design, timeline, `PRINT_LAYOUT`, export, and checklist: [docs/3d/adding-parts.md](../docs/3d/adding-parts.md).

## Commits

`type(mods): summary`. Name the mod in the summary (`feat(mods): add desk clamp`). One scope for every mod. `feat(mods)` / `fix(mods)` do not version the stock CAD revision. Moving a mod into stock [`3d_models/cad/`](../3d_models/cad/) and [`3d_models/parts/`](../3d_models/parts/) is `feat(cad)`. Full rules: [CONTRIBUTING.md](../CONTRIBUTING.md).

## License

CAD and manufacturing exports under `mods/<mod_name>/3d_models/{cad,parts}/` are [CERN-OHL-S-2.0](../3d_models/LICENSE). See [NOTICE](../3d_models/NOTICE) for warranty and product-notice requirements.

When you distribute a Product based on a mod, cite that licence and use the mod’s own `3d_models` tree as the Source Location (for example `https://github.com/jamro/tiny-engineer/tree/main/mods/halloween/3d_models`).

This README and other files beside `3d_models/` are documentation under the [MIT License](../LICENSE).
