# USB serial control

Tiny Engineer can be driven over USB CDC (same port used for flash) with a framed protocol. Use this when corporate Wi‑Fi blocks the robot, or when the device is provisioned as **USB-only**.

## Two setup surfaces (do not conflate)

| Surface | Choice | Persists |
| --- | --- | --- |
| Device AP wizard (web UI) **Connection** step | `link_mode`: `wifi` \| `usb` | NVS on the robot |
| Project `tiny-engineer setup` | host transport: `http` \| `serial` | project `.env` (`TINY_ENGINEER_TRANSPORT`) |

- Device `link_mode=usb` skips STA and does not start HTTP. Completing USB-only in the setup wizard **stops the setup AP in-session** and unlocks full serial without a power-cycle. Escape USB-only only via **factory reset** (reopens setup AP).
- Host transport is how the CLI/hooks talk to the robot. Default is `http`. Serial is opt-in via setup or env/flags.

## CLI transport

Resolution order: `--transport` → `TINY_ENGINEER_TRANSPORT` → default `http`. No auto-probe between HTTP and serial.

```bash
tiny-engineer setup cursor --yes --transport serial
# writes TINY_ENGINEER_TRANSPORT=serial (optional TINY_ENGINEER_PORT)

tiny-engineer anim ring --transport serial
tiny-engineer doctor --transport serial
tiny-engineer monitor          # prints LOG frames (exclusive port use)
tiny-engineer reset --yes      # factory reset; confirm or --yes
```

Serial port: `--port` / `TINY_ENGINEER_PORT`, else autodetect (Espressif VID `303A` preferred) with cache at `~/.config/tiny-engineer/serial-port.json` (invalidated on open/probe failure).

## Protocol overview

- **Framing:** COBS with `0x00` delimiter (vendored nanocobs on device).
- **Message header (7 bytes):** `type`, `ver`, `id` (u16 LE), `flags`, `len` (u16 LE), then payload.
- **Types:** req / res / log / evt / stream / sync. Control payloads are JSON; WAV play uses binary **stream** frames with a credit window.
- **Stream kinds:** data / credit / end / error. Rejected play chunks send **error** (reason text) instead of leaving the host blocked on credit wait. Firmware raises USB CDC RX/TX buffers so ~1 KB stream frames fit.
- **Provisioning:** while unprovisioned or in setup AP, only sync/ping. Full API after a connection mode is chosen (Wi‑Fi or USB-only). Serial does **not** use Bearer auth.
- **Busy:** long play holds a global busy lock; a second play (HTTP or serial) gets `busy`.

Ops (JSON `op`): `ping`/`sync`, `anim`, `settings.get`/`settings.set`, `test`, `servo`, `health`, `factory_reset`, `play` / `play_end`, plus stream data/credit/error frames.

## PlatformIO monitor

Stock PlatformIO serial monitor expects text. Framed COBS sessions look like noise. Prefer `tiny-engineer monitor` for LOG lines. Close the CLI monitor (and any other holder) before `pio run -t upload`. An optional PIO LOG filter is a follow-up, not required.

## Related

- HTTP API: [api.md](api.md)
- IDE hooks / CLI: [integration.md](integration.md)
- Settings / `link_mode`: [settings.md](settings.md)
