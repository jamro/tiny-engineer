# Proposal

## Why

Corporate networks often forbid unverified Wi‑Fi devices, so Tiny Engineer cannot rely on HTTP alone. USB CDC is already the flash/monitor path and is usually allowed; exposing the same control surface over a framed serial API lets the robot work air‑gapped while keeping Wi‑Fi mode for home use.

## What Changes

- Add a **USB CDC serial control plane**: typed messages (request, response, log, event, stream) framed with COBS; JSON for control; raw binary for WAV streaming; protocol version, correlation IDs, resync, and stream credit/flow control.
- Add a **connection mode** (`wifi` | `usb`) chosen at the end of the setup AP wizard: mode enables/disables Wi‑Fi and HTTP only; the serial API stays available whenever the device is provisioned (including in Wi‑Fi mode).
- Completing the Connection step (Wi‑Fi connect **or** USB-only) **ends the setup AP in-session** (softAP off, provisioning cleared); USB-only also stops HTTP and unlocks full serial without requiring a power-cycle.
- While provisioning (setup AP / unprovisioned): serial offers **ping/identity only**; full serial API after a mode is chosen.
- **Auth skipped** on serial (trusted local channel); HTTP Bearer rules unchanged.
- Escape USB-only mode via **factory reset** only (reopens setup AP); factory reset also available over CLI with explicit confirmation.
- Extend **tiny-engineer CLI**: explicit `--transport http|serial` (or env; default `http`, no auto-probe between transports); serial port autodetect + cache when transport is serial; commands covering day-to-day web UI parity (settings, tests, servo, health, factory reset, play over serial); `monitor` that decodes LOG frames.
- Extend interactive **`tiny-engineer setup`** (project/IDE hooks wizard): ask preferred host transport (`http` | `serial`), persist `TINY_ENGINEER_TRANSPORT` (and URL and/or optional serial port) in the project `.env` so later CLI commands and agent hooks use that transport without requiring flags. Flags still override. Distinct from the device AP Connection step (`link_mode`).
- Route application `serial_log` output through typed LOG frames (no raw `Serial.print` for app logs); CLI monitor preserves developer logging UX.
- Shared **command core** so HTTP and serial invoke the same operations (prepare for future device-initiated events).
- Review hardening (P0–P1): abort serial play sessions on stream failure (release busy; CLI best-effort `play_end`); reject empty / out-of-setup `link_mode` patches so USB-only escape stays factory-reset-only; keep large serial pack/decode buffers off the Arduino loop-task stack.

No **BREAKING** HTTP route removals in this change. New NVS/settings fields and setup UI choices are additive. USB-only mode turns HTTP off by design for that configuration.

## Capabilities

### New Capabilities

- `serial-protocol`: Framed USB CDC application protocol (COBS, typed messages, versioning, correlation, resync, streaming with flow control, LOG frames, future events).
- `connection-mode`: Provisioning link mode (`wifi` | `usb`), gates on Wi‑Fi/HTTP vs provisioned readiness, factory-reset escape from USB-only.
- `cli-transport`: Host CLI transport selection, serial port discovery/cache, robot command parity with the web UI, and serial monitor for LOG frames.

### Modified Capabilities

- (none — no existing OpenSpec specs in the project yet)

## Impact

- **Firmware**: new serial protocol stack; command core extracted/shared with HTTP handlers; settings/NVS for `link_mode`; setup wizard Network step; boot path skips STA/HTTP in USB mode; `serial_log` → LOG frames; vendored nanocobs (or equivalent lightweight COBS).
- **Web UI**: setup last step offers Wi‑Fi vs USB-only; API/docs mention connection mode.
- **Packages**: `tiny-engineer-cli` transport layer and setup/uninstall `.env` merge for transport; hook/anim/play/doctor use selected transport; agent packages that call HTTP helpers need shared client or CLI path.
- **Docs**: `docs/api.md`, integration, settings, hardware serial notes; PlatformIO monitor limitations + CLI monitor.
- **Dependencies**: vendored COBS on device; host serialport + COBS (e.g. npm `nanocobs` or equivalent); no heavy new firmware frameworks.
