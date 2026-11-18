# Tasks

## 1. Settings rename

- [x] 1.1 Rename settings symbols/defaults (`lcd1602BacklightSleep`, `SETTINGS_DEFAULT_LCD1602_BACKLIGHT_SLEEP`, getter) across `settings.h` / `values.*` / `getters.cpp` / `nvs.cpp`; keep NVS key `ext_lcd_bl`; verify `pio test -e native` still passes
- [x] 1.2 Switch HTTP and serial settings JSON to `lcd1602_backlight_sleep` (handlers, serial link, apply-immediate checks in `commands.cpp`); verify GET/POST/serial get/set no longer emit or require `lcd1602_backlight_boot`
- [x] 1.3 Update `docs/settings.md` field and NVS mapping lines for the new key; verify the listed HTTP name matches `docs/api.md` after docs sync

## 2. Backlight behavior

- [x] 2.1 Change `applyBrightnessPolicy` so non-`last` enable/boot always turns backlight on (default brightness); verify enable with flag false leaves backlight on without waiting for `/brightness`
- [x] 2.2 Gate `lcd1602FadeBrightnessTo` call sites in `src/sleep.cpp` with `settingsLcd1602BacklightSleep()`; verify flag true still fades sleep→0 and wake→100, flag false leaves brightness unchanged across sleep/wake
- [x] 2.3 Run `pio run` after firmware edits and verify the build succeeds

## 3. UI and API docs sync

- [x] 3.1 Update UI types, Extensions draft/save, ApiReference, and `ui/test/robot.tsx` to `lcd1602_backlight_sleep` with label **Follow sleep**; verify `npm run lint --prefix ui` and `npm test --prefix ui` pass
- [x] 3.2 Update `docs/api.md`, `docs/serial.md`, and `docs/hardware/testing.md` settings tables/examples for the rename and follow-sleep behavior; verify wording matches the lcd1602 delta scenarios
- [x] 3.3 Update CLI parity key list (`packages/tiny-engineer-cli/src/commands/parity.js`) and any other hardcoded `lcd1602_backlight_boot` references; verify `rg lcd1602_backlight_boot` finds none outside archives/history

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive`).
- Commit as `feat(settings)!: …` with `BREAKING CHANGE:` footer for the dropped JSON key.
