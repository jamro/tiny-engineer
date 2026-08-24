# Tiny Engineer

Imagine your AI coding agent had a tiny body and worked at the desk next to you.

Tiny Software Engineer is a physical avatar for an AI coding agent. It reads, thinks, types, and reacts as the agent works — turning invisible software activity into the behavior of a tiny teammate sitting beside you.

Instead of watching a progress spinner, just glance at your desk and see your AI at work.

---

Tiny Engineer is a small Wi-Fi desk robot that acts out your AI coding assistant while it works.

When you ask Cursor, Claude Code, or another agent to do a task, the model is busy somewhere in the cloud. On your desk, Tiny Engineer pretends it is the one doing the job: typing on a keyboard, leaning in to read, pausing to think, then getting back to work.

![Tiny Engineer at its desk](docs/tiny-engineer-preview.png)

## Hardware

Controller is a **Waveshare ESP32-C3-Zero**. Servos, display, and audio wiring live in `[docs/hardware/](docs/hardware/README.md)`. Robot layout and servo axes: `[docs/robot-movement.md](docs/robot-movement.md)`.

## Build and flash

Arduino firmware, built with [PlatformIO](https://platformio.org/). Boot inits onboard RGB, SSD1306 OLED, Wi-Fi, PCA9685 (5 servos), and MAX98357A (I2S), then serves JSON on port 80.

On first boot (or after factory reset and a power-cycle), the board opens a setup Wi-Fi network (`TinyEngineer-XXXX`). Connect to it, open `http://192.168.4.1/config`, and enter your home Wi-Fi credentials. The OLED shows setup instructions during this step. WiFi is not editable from the normal Config page afterward — use factory reset to change it.

Once on your LAN, open `http://tiny-engineer.local/` for the **web UI**: configure settings, run hardware tests, and try animations without touching curl.

Prerequisites:

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation.html), or the PlatformIO extension in VS Code / Cursor
- ESP32-C3-Zero connected over USB

From the project root:

```bash
pio run                 # build
pio run -t upload       # flash the board
pio device monitor      # serial monitor (115200 baud)
```

Build and flash in one step:

```bash
pio run -t upload && pio device monitor
```

If several serial ports are present, pick one:

```bash
pio device list
pio run -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor --port /dev/cu.usbserial-XXXX
```

Board and baud rate live in `platformio.ini`. The platform package is [pioarduino](https://github.com/pioarduino/platform-espressif32) (Arduino-ESP32 3.x) so `ESP_I2S` and `rgbLedWrite` are available.

## Web UI

After Wi-Fi is configured, open `http://tiny-engineer.local/` (or the IP on the OLED) in a browser. The panel is the easiest way to:

- **Configure** device name, sleep timeout, continuous anim timeout, volume, welcome, loading screen, and optional access token
- **Run hardware tests** (audio, OLED, servos, LED) without curl
- **Trigger animations** to try poses before wiring AI hooks

WiFi credentials are set only in setup AP mode (`/config` on `http://192.168.4.1`).

## HTTP API

JSON on port 80. Prefer the web UI for day-to-day config and tests; use curl/scripts for automation. Full reference: [`docs/api.md`](docs/api.md). Settings guide: [`docs/settings.md`](docs/settings.md).

When WiFi credentials are not saved yet, control APIs (`/anim`, `/test/*`) return **503**. When an access token is set, JSON APIs require `Authorization: Bearer <token>` (`GET /auth` is always public).

### Status and settings

```bash
curl http://tiny-engineer.local/auth
curl http://tiny-engineer.local/health
curl http://tiny-engineer.local/settings
curl -X POST "http://tiny-engineer.local/settings?sleep_timeout=2"
curl -X POST "http://tiny-engineer.local/settings?volume=40"
curl -X POST "http://tiny-engineer.local/settings?continuous_timeout=10"
```

### Animations

`GET /anim` returns the current pose. `POST /anim?name=…` switches pose (min **1s** hold; latest pending request wins). Continuous poses (`typing`, `reading`, `thinking`) time out to `attention` then idle after `continuous_timeout`.

| `name` | Kind | Role |
| --- | --- | --- |
| `none` | idle | Parked pose |
| `typing` | continuous | Hands typing, body sway |
| `reading` | continuous | Head/neck scan, occasional key taps |
| `thinking` | continuous | Head/neck ponder poses |
| `welcome` | one-shot | Boot hello (+ `welcome.wav`) |
| `ring` | one-shot | Service-bell strike (+ `bell.wav`) |
| `attention` | hold ~1 min | “Look here” prompt (+ `attention.wav`) |
| `error` | hold ~1 min | Problem pose (+ `error.wav`) |
| `abort` | one-shot | Resigned abort (+ `abort.wav`) |

```bash
curl http://tiny-engineer.local/anim
curl -X POST "http://tiny-engineer.local/anim?name=typing"
curl -X POST "http://tiny-engineer.local/anim?name=thinking"
curl -X POST "http://tiny-engineer.local/anim?name=none"
```

### Hardware tests

**POST**-only (they move hardware). Or use the web UI buttons.

```bash
curl -X POST http://tiny-engineer.local/test/audio   # also: …/test/audio/bell
curl -X POST http://tiny-engineer.local/test/screen
curl -X POST http://tiny-engineer.local/test/movement
curl -X POST http://tiny-engineer.local/test/led
curl -X POST "http://tiny-engineer.local/test/servo?index=0&angle=90"
```

## Integrating with AI tools

Two paths: raw **REST** (`POST /anim`) for any IDE/script, or the **Cursor** CLI via `npx`. Overview: `[docs/integration.md](docs/integration.md)`. Cursor hooks detail: `[docs/hooks.md](docs/hooks.md)`. Full HTTP reference: `[docs/api.md](docs/api.md)`.

## Cursor hooks

Open this repo in Cursor with the robot on Wi-Fi. Project hooks under `.cursor/` run `[tiny-engineer-cursor](packages/tiny-engineer-cursor/)` (via HTTPS GitHub tarball `npx`, or local `node` while developing). Setup and event map: `[docs/hooks.md](docs/hooks.md)`.

## License

MIT. See [LICENSE](LICENSE).