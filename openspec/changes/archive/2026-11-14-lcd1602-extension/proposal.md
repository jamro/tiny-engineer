# Proposal

## Why

The Waveshare LCD1602 on the I2C extension port only has a temporary boot smoke test. Users need an opt-in extension (like push-button) that can show agent/status text over REST, with a reusable `/extensions/<id>/…` HTTP pattern for later extensions. Extension code must live in a clear per-extension tree so push-button and LCD (and future add-ons) do not sprawl across `display/` and flat `extensions/` files.

## What Changes

- **Remove** the always-on `initLcd1602Hello()` smoke test (and its files) first, then add a proper **LCD 1602** extension, **off by default**, toggled on the web UI **Extensions** page (immediate apply, no reboot).
- **Restructure** firmware extensions under `src/extensions/<id>/` (one folder per extension). Move the existing push-button sources into `src/extensions/push_button/`. Keep shared infra (`pin_claims*`, HTTP registry) at `src/extensions/` root. LCD driver and its HTTP handlers live under `src/extensions/lcd1602/` — not under `src/display/`.
- When enabled: drive Waveshare LCD1602 (AiP31068L @ `0x3E`, SN3193 backlight @ `0x6B` — addresses fixed in firmware). When disabled: perform **no** LCD I2C traffic (module may be unplugged).
- Add boot settings: backlight on start (or wait for API); boot content `none` | `last` | `config` (with configured line strings for `config`).
- Persist **last** display state (both lines + per-line align + brightness) after every successful text/clear/brightness API call; restore when `boot=last`.
- Expose REST under `/extensions/lcd1602/…`: `GET`/`POST` text, `DELETE` text (clear both lines), `GET`/`POST` brightness. Partial JSON text updates; per-line align; empty `text` clears that line; lines longer than 16 characters marquee LTR (`[i..i+16)`, `i++`).
- Introduce a small **extension HTTP route registry** so future extensions mount under `/extensions/<id>/…` the same way (access + disabled → 503).
- No events from this extension. No configurable I2C addresses in v1. No PCF8574 / generic HD44780 backpack support.

No **BREAKING** HTTP route or settings semantics. Push-button behavior unchanged (path/include move only). New LCD settings and routes are additive; LCD defaults off.

## Capabilities

### New Capabilities

- `extension-http`: Shared pattern for mounting extension REST routes under `/extensions/<id>/…`, including access checks and the disabled-extension refusal.
- `lcd1602`: Optional Waveshare LCD1602 I2C extension — settings, Extensions UI, boot/last content, text/brightness HTTP API, scroll, and fixed-address driver.

### Modified Capabilities

- (none)

## Impact

- **Firmware**: remove smoke-test boot HELLO; nest `push_button/` and add `lcd1602/` under `src/extensions/`; update includes + native `build_src_filter` paths; LCD driver + apply/poll + handlers inside `lcd1602/`; NVS settings + last-state blob; shared HTTP registry; `kRoutes` / `isKnownPath` aware of `/extensions/<id>/…`.
- **Web UI**: second Extensions section “LCD 1602” (optional split into `ui/src/extensions/` section components); settings types; API reference rows for new routes and settings fields.
- **Docs**: `docs/api.md`, `docs/settings.md`, hardware testing / README summaries if endpoint lists change; note J3 I2C wiring and Waveshare-only module; short note on `src/extensions/` layout.
- **Tests**: native settings validation; update push-button / pin-claims include paths; UI tests as needed.
- **Dependencies**: none new (raw Wire to AiP31068/SN3193).
