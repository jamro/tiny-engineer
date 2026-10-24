# Proposal

## Why

The push-button extension already publishes five outbound events, but the robot stays silent. Users want optional local SFX per event (none / bell / click) so a press feels immediate on the desk without wiring host-side audio, while defaults stay quiet.

## What Changes

- Add five flat settings on `GET`/`POST /settings`: `push_button_sound_down`, `push_button_sound_up`, `push_button_sound_click`, `push_button_sound_double_click`, `push_button_sound_long_click`. Each accepts only `none`, `bell`, or `click`; default **`none`** for all (including factory reset). Apply immediately; persist even when `push_button` is false.
- When the extension publishes a button event, if that event’s setting is not `none` and the speaker is free, start the mapped LittleFS clip (`/bell.wav` or `/click.wav`) without blocking the loop. If the speaker is already busy (stock WAV stream, `/play`, or other active playback), **skip** the SFX; the outbound event still publishes.
- Pack `assets/click.wav` into the LittleFS image and register it as a playable clip alongside `bell.wav`.
- Extensions UI (toggle on): replace the plain event bullet list with a compact Events table — short label + select (`None` / `Bell` / `Click`) per event; keep wiring hint, outbound note, and gesture timing hints. Toggle off: hide table with pin/wiring as today.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `push-button`: Per-event sound settings, default none, play-on-publish with skip-when-busy, pack/play `click.wav`, compact Extensions Events table UX.

## Impact

- Settings layers per `docs/settings.md` (five string fields, NVS keys ≤15 chars, validation, getters, HTTP patch/JSON).
- Firmware: `pushButtonPoll` / publish path; audio busy query + non-blocking SFX start; `WavClip` / `STOCK_WAVS` / `assets/README.md` for click.
- UI: `Extensions.tsx`, `api/types.ts`, `ApiReference.tsx`; small CSS for the event×select rows.
- Docs: `docs/api.md`, `docs/settings.md` (and any GET `/settings` field lists that already mention push-button).
- Tests: settings validation for allowed values / reject unknown; UI draft/save if covered; native settings tests where push-button settings already live.
