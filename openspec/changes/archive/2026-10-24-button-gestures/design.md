# Design

## Context

See proposal.md — Why. The push-button extension already debounces GPIO20 edges in GPIO-free `PushButtonLogic` and publishes `button.down` / `button.up` from `pushButtonPoll()`. ISR ring and 25 ms debounce stay as-is. Gestures need a time-based FSM on top of accepted edges, plus poll ticks with “now” for deadlines while held or waiting for a second press.

## Goals / Non-Goals

**Goals:**

- Layer gesture recognition in the existing logic module so native tests stay GPIO-free.
- Publish gestures through the same `publishEvent` path as edges.
- Keep raw edges immediate; delay only the semantic `click`.

**Non-Goals:**

- NVS / settings for thresholds (fixed constants this change).
- Triple-click, repeat-while-held, or host-side gesture libraries.
- Changing the outbound envelope or WebSocket/serial transport.
- Removing or renaming `button.down` / `button.up`.

## Decisions

1. **Extend `PushButtonLogic`, do not replace debounce**  
   `accept(sample)` still returns Down/Up. Add gesture phase state and a `tick(uint64_t nowUs)` (or equivalent) that returns zero or more gesture events. `pushButtonPoll` drains the ring, then calls tick with `esp_timer_get_time()` so long/click fire without new edges.  
   *Alternatives:* separate gesture class — more files for little gain; host-only gestures — rejected in explore (model A on device).

2. **Fixed timings**  
   `kPushButtonLongHoldUs = 800e3`, `kPushButtonDoubleWaitUs = 350e3`. Document in `docs/api.md`.  
   *Alternatives:* settings knobs — defer; avoids settings-layer checklist this change.

3. **Event enum / publish mapping**  
   Extend or add a gesture enum (`Click`, `DoubleClick`, `LongClick`) distinct from edge events so one poll can emit Up then later tick emits Click. Wire names: `button.click`, `button.double_click`, `button.long_click`.

4. **FSM sketch**  
   States roughly: Idle → Holding (after Down) → either LongArmed (after long_click published) → Idle on Up; or after short Up → WaitSecond → Idle on timeout (click) or on second Down → Holding again (double candidate) / long on second hold cancels pending click. Disable/`apply(false)` clears pending click and long state with no synthetic gestures.

5. **Docs / UI only**  
   List new types next to down/up in `docs/api.md` and Extensions page copy. No ApiReference route change (no new HTTP).

## Risks / Trade-offs

- [Delayed click feels slow] → 350 ms is the confirmed UX trade for a clean single vs double contract; raw down/up remain immediate for low-latency hosts.
- [Play blocks tick] → Already mitigated: `pushButtonPoll` runs from play body callbacks; tick rides that same call.
- [Queued edges with stale timestamps vs wall “now”] → Long/click deadlines use sample timestamps for hold duration from Down, and `nowUs` for wait-after-up; document that overflow-dropped edges can still break gesture sequences the same way they break raw pairs.
- [Native tests grow complex] → Table-driven sequences with explicit `us` and `tick(now)` steps; keep scenarios aligned to spec.

## Migration Plan

- Firmware-only additive events; flash as usual. No NVS migration.
- Rollback: previous firmware stops sending gesture types; hosts that ignore unknown `type` keep working.

## Open Questions

None material for this change. Threshold tuning can be a later settings change if field testing demands it.
