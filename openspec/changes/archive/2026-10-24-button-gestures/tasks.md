# Tasks

## 1. Gesture logic (GPIO-free)

- [x] 1.1 Add fixed timing constants (long hold 800 ms, double wait 350 ms), gesture event types, and a `tick(nowUs)` path on `PushButtonLogic` that emits `click` / `double_click` / `long_click` per the push-button delta, and verify new native tests cover delayed click, double_click without click, long on hold then up-only, second-press-becomes-long canceling pending click, and disable clearing pending gestures (`pio test -e native`)
- [x] 1.2 Keep existing debounce edge tests green after the FSM change, and verify `pio test -e native` still passes the prior press/release, bounce, enable-while-held, and queued-edge cases

## 2. Firmware publish path

- [x] 2.1 Wire `pushButtonPoll` to publish gesture events from tick (and edges from accept) via `publishEvent` with types `button.click` / `button.double_click` / `button.long_click`, and verify `pio run` compiles
- [x] 2.2 Confirm poll+tick still runs from `loop()` and `POST /play` raw callbacks so long/click deadlines advance during play, and verify both call sites invoke `pushButtonPoll`

## 3. Docs and UI copy

- [x] 3.1 Document the three gesture types, 800 ms / 350 ms rules, and that down/up remain immediate in `docs/api.md` (outbound events and/or `push_button`), and verify the note matches the spec
- [x] 3.2 Update the Extensions page blurb to list gesture events alongside down/up, and verify `npm run lint --prefix ui` passes

## Workflow follow-up

- Archive the change after review (`/opsx-archive`).
