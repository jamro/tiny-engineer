# Tasks

## 1. Remove LCD smoke test

- [x] 1.1 Delete the temporary LCD test path before building the extension: remove `initLcd1602Hello()` call from `main.cpp`, delete `src/display/lcd1602.cpp` / `src/display/lcd1602.h`, and drop smoke-only LCD address constants from `include/pins.h` if nothing else needs them yet; verify `rg` finds no `initLcd1602Hello` / `LCD1602: HELLO` and `pio run` still succeeds

## 2. Extension tree layout (incl. push-button)

- [x] 2.1 Move push-button sources into `src/extensions/push_button/` (`push_button`, `logic`, `sfx`), update all `#include` sites, `platformio.ini` native `build_src_filter`, and native tests; verify `pio test -e native` and `pio run` pass with unchanged push-button behavior
- [x] 2.2 Add `src/extensions/README.md` stating the rule (per-extension code under `src/extensions/<id>/`; shared infra at `src/extensions/` root; OLED/eyes stay in `src/display/`) and verify the README matches the on-disk layout after the move
- [x] 2.3 Keep `pin_claims*` at `src/extensions/` root (shared); verify pin-claim native tests still include the correct path

## 3. Settings foundation

- [x] 3.1 Add `lcd1602`, `lcd1602_backlight_boot`, `lcd1602_boot`, `lcd1602_line1`, `lcd1602_line2` through the settings checklist (defaults, NVS keys ≤15 chars, validate, getters, patch, GET/POST JSON) and verify `pio test -e native` covers validation (boot enum, line length ≤64, defaults false/true/none/empty)
- [x] 3.2 Add last-state NVS blob load/save (both lines text+align + brightness; factory reset clears to empty/left + documented default brightness) and verify reset + reload round-trip in native or focused firmware unit coverage where practical
- [x] 3.3 Update `docs/settings.md` field/NVS lists for the new keys and verify the listed key names match `nvs.h`

## 4. LCD driver and extension runtime (`src/extensions/lcd1602/`)

- [x] 4.1 Add AiP31068/SN3193 `hw` helpers (fixed `0x3E`/`0x6B` in `pins.h` as needed) under `src/extensions/lcd1602/` only — not `src/display/` — and verify `pio run` compiles
- [x] 4.2 Implement `lcd1602ApplyFromSettings` / disable path (no I2C when off; probe+init+boot mode when on) in `src/extensions/lcd1602/` and verify serial log shows skip when off and init/boot behavior when on (or soft 503 path when NACK)
- [x] 4.3 Implement logical state, align ≤16, LTR marquee `i++` with readable fixed step/pause, and `lcd1602Poll` from `loop()`; verify long line scrolls and short aligned lines stay static on hardware or with a deterministic scroll-index unit test
- [x] 4.4 Wire apply on settings save and boot after `Wire.begin`; verify enable/disable via `POST /settings` applies without reboot

## 5. Extension HTTP registry and LCD API

- [x] 5.1 Add shared `http_registry` at `src/extensions/` root (`/extensions/<id>/<suffix>`, `Configured` access, disabled → 503, `isKnownPath` for 405), wired from `registerHttpRoutes`; verify a disabled LCD call returns 503 with no I2C side effects
- [x] 5.2 Implement `GET`/`POST`/`DELETE /extensions/lcd1602/text` handlers under `src/extensions/lcd1602/http.*` (partial JSON, empty text clears line, persist last on success, 400 on bad body, 503 if LCD NACK on mutate) and verify with curl or UI-less HTTP checks against the documented examples
- [x] 5.3 Implement `GET`/`POST /extensions/lcd1602/brightness` in the same `lcd1602` http module (0–100, persist last, 400/503 rules) and verify GET mirrors last successful value
- [x] 5.4 Sync `docs/api.md`, `ui/src/views/ApiReference.tsx`, and README / `docs/hardware/testing.md` endpoint summaries for the new routes; verify path/method/param text matches handlers

## 6. Extensions UI

- [x] 6.1 Add **LCD 1602** section on Extensions (toggle + backlight boot + boot mode + config lines when relevant + Waveshare I2C wiring hint + API note, no events); optionally extract push-button / LCD into `ui/src/extensions/*Section.tsx`; verify Config page has no LCD controls
- [x] 6.2 Extend `ui` Settings types and tests/fixtures for the new fields; verify `npm run lint --prefix ui` and `npm test --prefix ui` pass

## 7. Integration check

- [x] 7.1 Run `pio run` and confirm firmware builds clean after the full change

## Workflow follow-up

- Archive the change after review when implementation matches the specs.
- Verify the archived main specs for `extension-http` and `lcd1602` after archive.
- Hardware smoke (LCD on J3 + push-button after move) is manual — outside this task list.
