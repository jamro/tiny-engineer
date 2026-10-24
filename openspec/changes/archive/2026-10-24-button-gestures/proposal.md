# Proposal

## Why

Hosts that automate from the extension button need semantic gestures (`click`, `double_click`, `long_click`), not only raw edges. Edge events stay useful for hold-while-pressed logic; gestures give agents a clean contract without reimplementing timing on every client.

## What Changes

- Keep publishing `button.down` and `button.up` immediately on debounced edges (unchanged).
- Add firmware gesture recognition that also publishes `button.click`, `button.double_click`, and `button.long_click` on the same outbound sinks (`source` `push-button`).
- `long_click` fires **on hold** after **800 ms**; a later release publishes only `button.up` (no click / double_click for that press).
- Single `click` is **delayed** by a **350 ms** window after a short release so a second short press becomes `double_click` instead.
- Thresholds are fixed in firmware (not settings) for this change.
- Document the new event types in API docs and the Extensions UI copy.
- Extend native tests for the gesture state machine.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `push-button`: Add gesture events alongside raw down/up, with fixed long-hold and double-click timing rules.

## Impact

- Firmware: `push_button_logic` (+ poll “now” for long/timeout), `push_button` publish path, native `test_push_button_logic`.
- Docs: `docs/api.md` outbound / push_button notes; UI Extensions blurb.
- No new HTTP routes, settings keys, or CLI commands. `watch` already prints any event line.
- Not **BREAKING**: existing down/up behavior remains; hosts ignore unknown types today.
