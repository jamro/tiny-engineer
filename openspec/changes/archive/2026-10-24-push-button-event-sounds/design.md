# Design

## Context

Push-button already publishes five events from `pushButtonPoll` via `publishEvent`. Stock WAVs live on LittleFS; `bell.wav` is a `WavClip`, but `assets/click.wav` is not packed. Playback today is either blocking (`playWavBlocking`) or animation-driven (`wavStreamStart` + `wavStreamUpdate` in the robot tick). `/play` holds `apiBusy` and writes PCM directly. See proposal.md for motivation; see the push-button delta for requirements.

## Goals / Non-Goals

**Goals:**

- Five validated string settings with immediate apply, wired end-to-end (NVS → HTTP → UI).
- Non-blocking SFX on publish when free; never interrupt or queue behind busy audio.
- Pack and play `/click.wav`; reuse `/bell.wav`.
- Compact Extensions Events table (label + select).

**Non-Goals:**

- Custom uploaded sounds, volume-per-event, or host-side sound config.
- Preview-play buttons or new HTTP test routes for button SFX.
- Changing gesture timing or outbound event envelope.
- Mixing button SFX under `/play` or anim clips.

## Decisions

### 1. Flat string settings + short NVS keys

HTTP/JSON names match the proposal (`push_button_sound_down`, …). NVS keys ≤15 chars, e.g. `btn_snd_dn`, `btn_snd_up`, `btn_snd_clk`, `btn_snd_dbl`, `btn_snd_lng`. Store the literal tokens `none` / `bell` / `click` (same pattern as `eyes_style` / `loading`). Reject unknown strings in `SettingsPatch::invalidField`.

**Alternative considered:** nested `push_button_sounds` object — cleaner JSON, but new settings-handler pattern; rejected for consistency with existing flat fields.

### 2. Busy = any active speaker path

Treat speaker as busy when either:

- a LittleFS `wavStream` is playing (`g_playing` — expose a clip-agnostic `wavStreamIsActive()` or reuse existing query), or
- `apiBusyHeld()` is true (`POST /play` / serial play).

Do **not** start button SFX when busy; do not call `stopAllWavPlayback`. Publish still runs first (or regardless of SFX outcome).

**Alternative considered:** interrupt current clip — rejected (explore decision). Queue SFX — rejected (latency / complexity).

### 3. Non-blocking SFX start from publish

On mapped event: if setting maps to a clip and speaker free → `wavStreamStart(clip)` only (same path as `startBellPlayback`). Today only animations call `wavStreamUpdate` (e.g. ring/welcome); button SFX can fire while idle, so **also pump `wavStreamUpdate` from `tickRobot()` (or equivalent always-on path) when a stream is active**. Do **not** use `playWavBlocking` (blocks loop, OLED “Playing…”). Do **not** change OLED for short SFX.

Add `WavClip::Click` → `/click.wav`; append `click.wav` to `STOCK_WAVS` in `scripts/audio_pack.py`; document in `assets/README.md`.

### 4. Hook at publish site

Extend `publishButtonEvent` (or a thin helper next to it) to: publish envelope, then lookup setting for that event enum, then maybe-start SFX. Keeps one place for all five event types (edges + gestures + tick deadlines).

### 5. Extensions UI: event×select table

When draft toggle on: Events table rows show `button.*` as `<code>`, a short when-it-fires line (press / release / timings), and a compact sound `<select>` (None / Bell / Click). Column header “Emitted event / Sound”. Save posts all five sound fields with pin + enable. Short outbound one-liner under the table. Keep markup compact for the gzipped UI.

### 6. Docs sync

Same change updates `docs/api.md`, `docs/settings.md`, `ui` types + ApiReference settings rows (no new HTTP routes → route table unchanged).

## Risks / Trade-offs

- [Bell ~2 s on down/up] → Mitigation: default none; user chooses; skip-when-busy limits pile-up; non-blocking so loop still polls.
- [Missed SFX during welcome/error/`/play`] → Mitigation: accepted; events still outbound for host automation.
- [Missing `/click.wav` on old FS] → Mitigation: pack in stock image; `wavStreamStart` failure is silent skip (event still published); users need `uploadfs` after upgrade.
- [NVS key proliferation] → Mitigation: five short keys; still under Preferences limits.

## Migration Plan

- Fresh / factory: all sounds `none` — no behavior change.
- Upgrade firmware + FS: new fields appear as `none`; flash FS once so `/click.wav` exists.
- Rollback: old firmware ignores unknown settings keys on POST if rejected as unknown field — clients should stop sending the new keys; NVS orphans are harmless until factory reset.

## Open Questions

None — busy policy, API shape, and UX locked in explore.
