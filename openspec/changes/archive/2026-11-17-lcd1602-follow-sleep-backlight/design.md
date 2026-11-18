# Design

## Context

See proposal.md — Why. Today `lcd1602_backlight_boot` only steers enable/boot brightness in `applyBrightnessPolicy` (`src/extensions/lcd1602/lcd1602.cpp`). Sleep/wake always call `lcd1602FadeBrightnessTo` from `src/sleep.cpp` when the extension is active. HTTP/serial/UI/docs all name the boot field. NVS short key is already `ext_lcd_bl`.

## Goals / Non-Goals

**Goals:**

- One boolean meaning: gate sleep/wake backlight fades; boot/enable still turns backlight on when not restoring from `last` (and when follow sleep is off, even if restored `last` brightness is 0).
- Rename the public JSON field; keep NVS storage cell.
- Spec, UI, HTTP, serial, and docs stay in sync (settings checklist + API sync).

**Non-Goals:**

- Changing fade duration (keep `anim::SLEEP_NOD_OFF_MS`).
- Restoring wake to pre-sleep brightness (always 100).
- Migrating or aliasing the old JSON key.
- New HTTP routes or outbound events.
- CLI host packages beyond serial/parity key lists that already hardcode the LCD settings slice.

## Decisions

1. **JSON rename, NVS reuse**  
   Public: `lcd1602_backlight_sleep`. Internal C++: rename symbols (`lcd1602BacklightSleep`, `settingsLcd1602BacklightSleep`, default `SETTINGS_DEFAULT_LCD1602_BACKLIGHT_SLEEP`). Keep `kKeyLcd1602Bl = "ext_lcd_bl"` so existing devices keep their stored bool without a migration step.  
   *Alt:* new NVS key — rejected (need migration + drop old).  
   *Alt:* keep JSON `…_boot` — rejected (misleading forever).

2. **Gate fades in sleep path**  
   Wrap `lcd1602FadeBrightnessTo(0|100, …)` call sites in `sleep.cpp` with `settingsLcd1602BacklightSleep()` (and keep existing `g_active` guard inside the fade helper).  
   *Alt:* gate inside `lcd1602FadeBrightnessTo` — rejected; that helper is also usable for future non-sleep fades and should stay a dumb fade.

3. **Boot policy always on (non-last), plus last-zero override when follow sleep off**  
   Replace the old if/else in `applyBrightnessPolicy`: when not applying `last` brightness, always set default brightness on (both flag true and false). Flag only gates sleep/wake fades.  
   `lcd1602_boot=last` still restores stored brightness, except when `lcd1602_backlight_sleep` is false and that brightness is **0** — then force default brightness on (sleep may have persisted 0 into last; follow-sleep-off means the panel stays lit while awake).

4. **Unknown old key**  
   Do not accept or emit `lcd1602_backlight_boot`. Never returned on GET.  
   - **HTTP** `POST /settings`: `readPatch()` rejects the whole body with **400** `unknown field lcd1602_backlight_boot` and saves nothing — same as any other unknown settings field.  
   - **Serial** `settings.set`: only reads known keys and ignores unknowns. A body that also has known fields still saves those; a body with **only** the removed key (no known fields) → **400** `send at least one setting`.  
   Clients must drop the old key before submitting a patch (especially over HTTP).

5. **UI copy**  
   Label: **Follow sleep**. Bound to `lcd1602_backlight_sleep`.

6. **Breaking commit**  
   `feat(settings)!: rename lcd1602 backlight setting` with footer `BREAKING CHANGE: lcd1602_backlight_boot removed; use lcd1602_backlight_sleep.` Scope `settings` (JSON + NVS mapping); firmware/UI/docs ride along in the same change.

## Risks / Trade-offs

- [Devices with `ext_lcd_bl=false`] → Semantics flip: was “off until API”, becomes “boot on, no sleep dim”. Acceptable; matches product intent and default remains true.  
- [External scripts still sending old key] → HTTP: **400** `unknown field lcd1602_backlight_boot`, entire patch rejected (including sibling fields). Serial: old key ignored; known siblings still save; only-old-key body → **400** `send at least one setting`. Mitigate: API docs + BREAKING CHANGE footer; tell clients to remove the key; ApiReference update.  
- [Wake always 100] → Overrides a manual dim set before sleep when follow sleep is on. Accepted (user decision).

## Migration Plan

- Flash new firmware: NVS bool reused; no wipe.
- Clients: use `lcd1602_backlight_sleep`; remove `lcd1602_backlight_boot` from POST/serial bodies (HTTP: unknown field → 400, nothing saved; serial: key ignored, or 400 if it was the only field).
- Rollback: old firmware again reads `ext_lcd_bl` as backlight-on-boot (false again means off until API).

## Open Questions

None — wake target 100, OFF = boot on + no sleep sync, rename to `lcd1602_backlight_sleep`, and UI **Follow sleep** confirmed in explore.
