# Proposal

## Why

The robot already publishes outbound events (for example `button.double_click`) and can play a per-button sound from fixed settings, but it cannot run general on-device reactions without a host. Users need event→action bindings that work offline on the firmware, managed from the robot API and web UI.

## What Changes

- Add an on-device **Actions** list: each row binds an event name/pattern to one action (`sound` or `anim`).
- Patterns support `*` as a wildcard for any character sequence in any position of the event type string; matching fires **all** matching rows.
- Actions:
  - **sound** — play `bell` or `click` (same LittleFS clips as today)
  - **anim** — trigger any animation the robot supports, with the same rules as `POST /anim`
- **BREAKING**: remove `push_button_sound_*` settings and the Extensions per-event sound selects; sound reactions move to Actions. Migrate non-`none` legacy sound settings into Actions rows on first load when the Actions list is empty.
- New HTTP API to list/replace Actions and to **simulate an event** for testing (run matching Actions without requiring a physical trigger).
- New web UI **Actions** tab for clear multi-row editing, save, and event simulation.
- Firmware / robot web UI / API only — **no** host daemon, CLI, or `rules.json` changes.

## Capabilities

### New Capabilities

- `event-actions`: On-device event→action bindings (storage, match/dispatch, sound/anim effects, Actions HTTP API, Actions web UI, simulate-event test).

### Modified Capabilities

- `push-button`: Drop per-event sound settings and Extensions sound selects; keep enable/pin/wiring/event catalog; publishing still goes through the shared outbound hub (Actions hook in on publish).

## Impact

- Firmware: `src/events/`, new actions module, settings/NVS list storage, HTTP routes/handlers, push-button SFX path removed in favor of Actions dispatch.
- Web UI: new Actions page/nav; Extensions page simplified; settings types and ApiReference updated.
- Docs: `docs/api.md`, settings docs, push-button/extensions docs; README/hardware testing summaries if endpoint lists change.
- Host packages / CLI / `host-event-reactions`: out of scope (unchanged).
- Breaking for clients that read/write `push_button_sound_*` on `/settings`.
