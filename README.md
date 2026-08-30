# Tiny Engineer

> Give your AI coding agent a body.

![Tiny Engineer demo](docs/tiny-engineer-preview.gif)

Tiny Engineer is an open-source, 3D-printable desktop robot that physically acts out what an AI coding agent is doing: reading, thinking, coding, finishing tasks. It runs on an ESP32 over Wi-Fi and exposes a simple REST API, so any tool that can make an HTTP request can drive it.

**[Watch the demo](https://youtu.be/RX_QRxdXMjg) · [Build your own](#build-your-own) · [How it works](#how-it-works) · [API](docs/api.md)**

## Why?

AI coding agents increasingly spend long stretches working on their own. I wanted to see what mine was doing without staring at the IDE.

So I built a slightly ridiculous desk robot: instead of another spinner or status line, there is a tiny teammate sitting next to me, visibly reading, thinking, and typing while the agent works.

## How it works

![How Tiny Engineer works](docs/how-it-works.jpg)

An AI agent works; an integration turns that into events; the robot’s REST API on your LAN receives them; the hardware moves and reacts.

**Two ways in**

- **Built-in** — Cursor Agent → Cursor hooks → `tiny-engineer-cursor` → HTTP over Wi-Fi
- **Bring your own** — Claude, Codex, or any other agent → your script/plugin/app → HTTP REST

Both hit the same Tiny Engineer REST API (`/anim`). The ESP32-C3 runs that API, robot logic, and animations, then drives:

| Bus | Hardware | Role |
| --- | --- | --- |
| I2C | PCA9685 → 5× servos | Head / neck, hands / body |
| I2C | SSD1306 OLED | Status, face, info |
| I2S | MAX98357A → speaker | Audio |

Cursor is one, sample, client, not the architecture. Details: [Cursor hooks](docs/hooks.md) · [any integration](docs/integration.md) · [HTTP API](docs/api.md)

## Build your own

End-to-end path (details live in the linked docs):

New to hardware? Start with **[From Code to Circuits](docs/hardware-for-software-engineers/README.md)** — hardware and electronics for software engineers; Tiny Engineer is the worked example.

1. **Get the electronics** — [hardware inventory](docs/hardware/components.md)
2. **3D print the parts** — [printables](3d_models/README.md)
3. **Assemble the mechanics** — fit the five servos — [3D models](3d_models/README.md), [servo axes](docs/robot-movement.md)
4. **Wire the electronics** — [wiring](docs/hardware/wiring.md), [hardware overview](docs/hardware/README.md)
5. **Flash the ESP32 firmware** — [getting started → Flash](docs/getting-started.md#4-flash)
6. **Configure Wi-Fi** — [getting started → Wi-Fi](docs/getting-started.md#5-wi-fi-setup)
7. **Test the robot** — web UI + a curl — [getting started → Prove it](docs/getting-started.md#6-prove-it)
8. **Connect your coding agent** — [Cursor hooks](docs/hooks.md) or [any IDE / REST](docs/integration.md)

Full walkthrough: [docs/getting-started.md](docs/getting-started.md).

## Hardware overview

Major pieces (exact models and counts in the BOM):

| Role | Part | Qty |
| --- | --- | --- |
| Controller | Waveshare ESP32-C3-Zero | 1 |
| Servo PWM | Adafruit PCA9685 | 1 |
| Actuators | PowerHD HD-1370A micro servos | 5 |
| Display | 0.91" 128×32 SSD1306 OLED (I2C) | 1 |
| Audio | MAX98357A + 8 Ω / 1 W speaker | 1 + 1 |
| USB / power | Adafruit 5993 USB-C breakout; **5 V / ≥2 A** supply | 1 |
| Structure | 3D-printed parts | — |

Complete inventory and limits: [docs/hardware/components.md](docs/hardware/components.md).

## Quick start

Firmware is Arduino on [PlatformIO](https://platformio.org/). From the project root:

```bash
pio run                 # build
pio run -t upload       # flash firmware + LittleFS
pio device monitor      # serial (115200)
```

If servos move but audio is silent, run `pio run -t uploadfs` once.

**Wi-Fi (first boot):** join `TinyEngineer-XXXX`, open `http://192.168.4.1/config`, enter a **2.4 GHz** network. OLED shows the setup steps.

**Web UI:** `http://tiny-engineer.local/` (or the IP on the OLED) — settings, hardware tests, animations.

**Make it move:**

```bash
curl -X POST "http://tiny-engineer.local/anim?name=ring"
```

Full route list: [docs/api.md](docs/api.md). Settings: [docs/settings.md](docs/settings.md).

## AI integrations

Anything that can `POST` over HTTP can drive the robot:

```bash
curl -X POST "http://tiny-engineer.local/anim?name=typing"
```

Useful `name` values: `typing`, `reading`, `thinking`, `ring`, `welcome`, `wakeup`, `sleep`, `attention`, `error`, `abort`, `dead`, `none`.

Cursor projects can map agent events to poses via hooks — [docs/hooks.md](docs/hooks.md). Broader patterns and examples: [docs/integration.md](docs/integration.md).

## Documentation

| Goal | Doc |
| --- | --- |
| Build end-to-end | [docs/getting-started.md](docs/getting-started.md) |
| Parts / BOM | [docs/hardware/components.md](docs/hardware/components.md) |
| Wiring / power | [docs/hardware/README.md](docs/hardware/README.md) |
| Printable parts | [3d_models/README.md](3d_models/README.md) |
| Servo axes / safe ranges | [docs/robot-movement.md](docs/robot-movement.md) |
| HTTP API | [docs/api.md](docs/api.md) |
| Settings | [docs/settings.md](docs/settings.md) |
| Cursor hooks | [docs/hooks.md](docs/hooks.md) |
| Any IDE / REST | [docs/integration.md](docs/integration.md) |
| Full index | [docs/README.md](docs/README.md) |

## Build one, break one, share it

Print it, wire it, change the CAD, swap animations, or hook up a different agent. Issues and PRs welcome — especially new integrations.

![Tiny Engineer](docs/tiny-engineer-preview.jpg)

## License

MIT. See [LICENSE](LICENSE).
