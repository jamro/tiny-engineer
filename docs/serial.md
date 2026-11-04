# USB serial control

Tiny Engineer can be driven over USB CDC (same port used for flash) with a framed protocol. Use this when corporate Wi‑Fi blocks the robot, or when the device is provisioned as **USB-only**.

## Two setup surfaces (do not conflate)

| Surface | Choice | Persists |
| --- | --- | --- |
| Device AP wizard (web UI) **Connection** step | `link_mode`: `wifi` \| `usb` | NVS on the robot |
| `tiny-engineer setup` | host transport: `http` \| `serial` | user config `env` (`TINY_ENGINEER_TRANSPORT`) |

- Device `link_mode=usb` skips STA and does not start HTTP. Completing USB-only in the setup wizard **stops the setup AP in-session** and unlocks full serial without a power-cycle. Escape USB-only only via **factory reset** (reopens setup AP).
- Host transport is how the CLI/hooks talk to the robot. Default is `http`. Serial is opt-in via setup or env/flags.

## CLI transport

Resolution order: `--transport` → process/project/user `TINY_ENGINEER_TRANSPORT` → default `http`. No auto-probe between HTTP and serial. Project `.env` overrides user config when present.

```bash
tiny-engineer setup cursor --yes --transport serial
# writes TINY_ENGINEER_TRANSPORT=serial to user config (optional TINY_ENGINEER_PORT)

tiny-engineer anim ring --transport serial
tiny-engineer doctor --transport serial
tiny-engineer monitor          # prints LOG frames from the host daemon
tiny-engineer port             # serial ports and which one auto-detect would open
tiny-engineer reset --yes      # factory reset; confirm or --yes
```

Serial port: `--port` / `TINY_ENGINEER_PORT`, else the cache, else autodetect (Espressif VID `303A` preferred). `tiny-engineer port` prints that choice and every host port's score. `tiny-engineer port --probe` sends `sync` and does not write the cache. A successful serial open still writes `serial-port.json` and drops it when open or `sync` fails.

## Host daemon

Serial commands share one host process, `tiny-engineer daemon`. The first serial command auto-starts it, unless the daemon is disabled or the port is yielded. Closing the terminal does not stop it. `watch` and `monitor` subscribe to that process. Ctrl+C on either one leaves the daemon running. A live process is assured only after `tiny-engineer daemon install`. Without that, a crash stays down until the next serial command.

`tiny-engineer daemon release` and plain `tiny-engineer daemon stop` close the USB port and leave the process running. The next serial command may open the port again. Both also suspend serial reaction auto-open: the daemon does not take the port back for `rules.json` until `tiny-engineer daemon start`, `tiny-engineer daemon restart`, or a serial command opens it. `tiny-engineer daemon stop --yield` leaves the process running, refuses serial open for 180 seconds, and does not reopen the port when that window ends. `tiny-engineer daemon start` clears a yield. When serial reactions are armed (at least one rule in `rules.json` and user transport `serial`), `start` and `restart` open the USB port. With no rules, or with transport `http`, they leave the port closed. HTTP reactions stay connected across `release`, `stop`, and `stop --yield`. `tiny-engineer daemon disable` exits the process and refuses auto-start until `tiny-engineer daemon start`, which ends reactions until the next start. HTTP `anim` and hooks do not go through the daemon.

`tiny-engineer daemon install` registers a per-user login wrapper. It is not a system-boot service. The wrapper is what restarts the process after a crash. `tiny-engineer daemon uninstall` removes that wrapper and does not disable auto-start.

`tiny-engineer daemon status` prints whether `daemon install` is on, the yield timer when a yield is active, plus pid, version, channel (`npm` or `dev`), package root, protocol, port, reaction link, rule count, and the last reaction. It also lists the host serial ports and marks the one the daemon has open. The install line and the port list are there even when the process is down. A registry install (`npx -y tiny-engineer` or a global install) is channel `npm`. Running `node packages/tiny-engineer-cli/bin/tiny-engineer.js` from this repo is channel `dev`. A dev checkout replaces a running npm daemon. A published npm command does not replace a dev daemon. The same npm version keeps the process that is already running. `daemon status` shows which one you have. `daemon restart` from the checkout you want replaces a different dev tree.

Before another program opens the USB serial device, run `tiny-engineer daemon release` or plain `tiny-engineer daemon stop`. Firmware upload parks servos, then runs `tiny-engineer daemon stop --yield` so auto-start stays off while the flash tool has the port. `tiny-engineer daemon start` clears that yield.

## Protocol overview

- **Framing:** COBS with `0x00` delimiter (vendored nanocobs on device).
- **Message header (7 bytes):** `type`, `ver`, `id` (u16 LE), `flags`, `len` (u16 LE), then payload.
- **Types:** req / res / log / evt / stream / sync. Control payloads are JSON; WAV play uses binary **stream** frames with a credit window.
- **Stream kinds:** data / credit / end / error. Rejected play chunks send **error** (reason text) instead of leaving the host blocked on credit wait. Firmware raises USB CDC RX/TX buffers so ~1 KB stream frames fit.
- **Provisioning:** while unprovisioned or in setup AP, only sync/ping and `servos_park`. Full API after a connection mode is chosen (Wi‑Fi or USB-only). Serial does **not** use Bearer auth.
- **Busy:** long play holds a global busy lock; a second play (HTTP or serial) gets `busy`.

Ops (JSON `op`): `ping`/`sync`, `servos_park`, `anim`, `settings.get`/`settings.set`, `test`, `servo`, `health`, `factory_reset`, `play` / `play_end`, plus stream data/credit/error frames.

## PlatformIO monitor

Stock PlatformIO serial monitor expects text. Framed COBS sessions look like noise. Prefer `tiny-engineer monitor` for LOG lines. The pre-upload step parks servos, then runs `daemon stop --yield` so the host daemon releases the port for the flash tool. Close any other serial program before `pio run -t upload`. An optional PIO LOG filter is a follow-up, not required.

## Related

- HTTP API: [api.md](api.md)
- IDE hooks / CLI: [integration.md](integration.md)
- Settings / `link_mode`: [settings.md](settings.md)
