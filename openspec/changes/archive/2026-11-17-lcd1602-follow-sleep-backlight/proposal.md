# Proposal

## Why

The LCD1602 Extensions toggle is labeled **Backlight on boot**, but firmware already dims/lights the backlight on sleep and wake regardless of that flag. The label and setting name describe the wrong behavior, and the false case (“stay off until `/brightness`”) fights the sleep/wake path users actually notice.

## What Changes

- Replace `lcd1602_backlight_boot` with **`lcd1602_backlight_sleep`** (default **true**).
- UI label becomes **Follow sleep**.
- **When true:** backlight on at enable/boot (unless boot content `last` supplies brightness); sleep fades to **0**; wake / wake-from-sleep fades to **100**.
- **When false:** backlight on at enable/boot (unless `last` supplies a non-zero brightness); if `last` brightness is **0**, still turn on. Sleep and wake **do not** change backlight.
- **BREAKING:** HTTP and serial settings drop `lcd1602_backlight_boot`; clients must use `lcd1602_backlight_sleep`. NVS key `ext_lcd_bl` is reused (stored bool kept).
- Remove the old “false → off until brightness API” policy.
- Document sleep/wake backlight gating in the lcd1602 capability (today only mentioned in API prose).

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `lcd1602`: Replace backlight-on-boot requirement with follow-sleep backlight; add sleep/wake fade behavior gated by the setting.
- `serial-protocol`: Serial settings LCD slice uses `lcd1602_backlight_sleep` instead of `lcd1602_backlight_boot`.

## Impact

- Firmware: settings field/getters/NVS mapping rename; `applyBrightnessPolicy`; gate fades in `src/sleep.cpp`.
- HTTP/serial settings JSON; Web UI Extensions + ApiReference + types/tests.
- Docs: `docs/api.md`, `docs/settings.md`, `docs/serial.md`, `docs/hardware/testing.md`.
- Native settings tests / fixtures that mention the old key.
- Commit as `feat(settings)!:` (or `feat(http)!:`) with `BREAKING CHANGE` footer for the dropped JSON key.
