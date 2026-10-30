# Tasks

## 1. Settings: five sound fields

- [x] 1.1 Add the five `push_button_sound_*` string settings (default `none`, allowlist `none`/`bell`/`click`, NVS keys ≤15 chars e.g. `btn_snd_*`) through the settings layers per `docs/settings.md`, expose on `GET`/`POST /settings`, and verify `pio test -e native` rejects invalid values and defaults to `none`
- [x] 1.2 Document the five fields in `docs/api.md`, `docs/settings.md`, and `ui/src/views/ApiReference.tsx` settings tables (defaults, allowlist, persist-when-off, immediate apply) and verify wording matches the delta spec

## 2. Audio: click clip + busy + pump

- [x] 2.1 Add `click.wav` to `STOCK_WAVS` / `WavClip::Click` / clip table, update `assets/README.md`, and verify `python3 scripts/test_audio_pack.py` packs `/click.wav` into the image
- [x] 2.2 Expose a clip-agnostic “stream active” query (or equivalent), treat busy as stream-active **or** `apiBusyHeld()`, add a non-blocking start helper for button SFX that skips when busy, pump `wavStreamUpdate` from `tickRobot()` when a stream is active, and verify `pio run` compiles

## 3. Play on publish

- [x] 3.1 From the push-button publish path, after publishing the event, map the event to its sound setting and maybe-start the clip; verify with a unit/logic test or documented assertion that `none` and busy paths never start audio while a free `bell`/`click` mapping does call start

## 4. Extensions UI

- [x] 4.1 Replace the Extensions event bullet list with a compact Events table (short label + `None`/`Bell`/`Click` select per event), include the five sound fields in draft/save, keep pin/wiring/outbound hints, hide with the toggle, update `ui/src/api/types.ts`, and verify `npm run lint --prefix ui` and `npm test --prefix ui` pass

## 5. Integration

- [x] 5.1 Run `pio run` after firmware groups and verify the default env links

## Workflow follow-up

- Archive the change after review (`/opsx-archive` or project equivalent).
- Sync delta specs into main specs on archive.
