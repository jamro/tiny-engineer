# Design

## Context

See proposal.md — Why. Today: flat `kRoutes[]` in `src/http/routes.cpp`; push-button files sit flat under `src/extensions/`; temporary `initLcd1602Hello()` lives under `src/display/`. Settings checklist lives in `docs/settings.md`. I2C bus already shared (PCA9685 `0x40`, OLED `0x3C`). Waveshare LCD1602 uses AiP31068L @ `0x3E` + SN3193 @ `0x6B` (fixed). Pin-claims do not apply to LCD (no GPIO).

## Goals / Non-Goals

**Goals:**
- Opt-in LCD 1602 extension with Extensions UI + NVS settings + last-state restore
- Reusable `/extensions/<id>/…` mount table for future extensions
- Clear **per-extension** firmware tree: `src/extensions/<id>/…`; push-button moved into that layout in the same change
- Raw Wire driver (no new PlatformIO lib) for AiP31068 + SN3193
- Scroll tick from `loop()` when a line is long

**Non-Goals:**
- Configurable I2C addresses or PCF8574 backpacks
- Outbound events / Actions triggers from LCD
- Multi-LCD or I2C mux
- Per-line brightness or RGB backlight modes beyond single brightness 0–100
- Moving NVS/settings core out of `src/settings/` into per-extension folders
- Changing push-button runtime behavior (move/includes only)

## Decisions

### 1. Extension HTTP registry beside `kRoutes`
Add a small table (e.g. `ExtHttpRoute`: `id`, `suffix`, method, `Access`, `enabled()` predicate, handler) in shared extension infra, registered from `registerHttpRoutes`. Paths become `/extensions/` + id + suffix. Gate: `Refusal::check(Configured)` then if `!enabled()` → **503** `extension disabled`. Extend `isKnownPath` so wrong methods on registered suffixes → **405**.

**Alternatives:** stuff every path into `kRoutes` (works once, no shared disabled gate); single catch-all `UriBraces("/extensions/{}/{}")` dispatcher (harder 405/table docs). Prefer explicit table rows for docs sync and typed handlers.

### 2. Keep SPA `GET /extensions` in `kRoutes`
Unchanged shell route. Extension APIs never collide with the page path (they need a second segment).

### 3. Per-extension module layout
**Rule:** code that exists only for one optional extension lives under `src/extensions/<id>/`. Shared extension infrastructure stays at `src/extensions/` root. Stock robot face (OLED/eyes) stays in `src/display/` — LCD is not face firmware.

```
src/extensions/
  README.md                 # layout rule for agents/humans
  pin_claims.*              # shared GPIO claims
  pin_claims_settings.*
  http_registry.*           # shared /extensions/<id>/ mount
  push_button/
    push_button.h/.cpp      # apply / poll
    logic.h/.cpp            # pure debounce/gestures (native-tested)
    sfx.h
  lcd1602/
    lcd1602.h/.cpp          # apply / poll / logical state / boot
    hw.h/.cpp               # AiP31068 + SN3193
    http.h/.cpp             # REST handlers registered via registry
```

- **First:** delete smoke test (`initLcd1602Hello`, `src/display/lcd1602.*`, smoke-only pins).
- **Then:** move push-button into `push_button/` and fix includes + `platformio.ini` native filter + tests.
- **Then:** add `lcd1602/` (hw + runtime + http). Call `lcd1602ApplyFromSettings()` after settings/I2C ready and `lcd1602Poll()` from `loop()`.
- Optional UI: `ui/src/extensions/*Section.tsx` imported by `Extensions.tsx` — same split, not required for v1 if the page stays readable.

**Alternatives rejected:** LCD hw under `src/display/` (blurs stock display vs optional extension); LCD handlers only under `src/http/` with no extension folder (scatters the feature); leave push-button flat (two layouts forever).

### 4. Settings / NVS
JSON fields: `lcd1602`, `lcd1602_backlight_boot`, `lcd1602_boot`, `lcd1602_line1`, `lcd1602_line2`. Short NVS keys under 15 chars (e.g. `ext_lcd`, `ext_lcd_bl`, `ext_lcd_boot`, `ext_lcd_l1`, `ext_lcd_l2`). Last state: one `putBytes` blob (two 64-char lines + aligns + brightness) keyed e.g. `ext_lcd_last`. Follow `docs/settings.md` checklist end-to-end. Defaults: enable false, backlight_boot true, boot `none`, lines empty.

### 5. Logical state vs viewport
RAM holds full strings (≤64) + align + brightness + scroll index per line. GET returns logical strings. Hardware shows 16-col window (aligned pad or marquee slice). Scroll step ~375 ms; ~750 ms pause when wrap returns to 0 (readable).

### 6. Success vs hardware failure
Validate JSON first. If LCD missing (`0x3E` NACK): **503** on mutating routes that need HW; do not update last blob on failed HW apply. If HW OK: update RAM, write display, save last, **200** with state. GET may return RAM/NVS even when HW absent.

### 7. Disable behavior
No I2C teardown writes (module may be gone). Stop poll; drop “active” flag. Last blob kept for later `boot=last`.

### 8. Access
`Configured` for all `/extensions/lcd1602/*` APIs (same as `/anim`).

### 9. Docs / UI sync
Per `.cursor/rules/sync-api-endpoints.mdc`: `routes` / extension table, `docs/api.md`, `ui/src/views/ApiReference.tsx`, settings tables, hardware testing / README if endpoint list changes. Document `src/extensions/` layout briefly (AGENTS.md or `src/extensions/README.md`).

## Risks / Trade-offs

- [Include-path churn for push-button] → Mitigation: mechanical move + `rg` for old paths; native tests must pass before LCD work.
- [SN3193 absent / different backlight] → Mitigation: LCD text still works if AiP31068 ACKs; brightness POST may 503 if LED NACK while LCD OK (document).
- [I2C bus load / slow Wire during scroll] → Mitigation: one short transaction per step; fixed modest period; never block HTTP long.
- [NVS wear from frequent text updates] → Mitigation: write last blob only when logical state actually changes.
- [Extension table vs docs drift] → Mitigation: same sync checklist as `kRoutes`.

## Migration Plan

1. Remove smoke-test code first (no HELLO on boot).
2. Move push-button into `src/extensions/push_button/`; fix includes/filters/tests; `pio run` + `pio test -e native`.
3. Land settings + `lcd1602/` + registry; LCD inert until `lcd1602` true.
4. No settings migration (new keys only).
5. Rollback: flash previous firmware; new NVS keys ignored by old builds.

## Open Questions

None that block specs/tasks. Scroll pause/step constants may be tuned on hardware during apply without changing the contract (“readable fixed pace”).
