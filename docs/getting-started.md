# Getting started

Build the robot end-to-end: parts → print → wire → flash → Wi‑Fi → one animation. Skip ahead if that stage is already done (e.g. robot on Wi‑Fi → jump to [hooks](hooks.md)).

## What is the ESP32?

**ESP32** is a family of small Wi‑Fi microcontrollers from Espressif. Tiny Engineer’s “brain” is one of these boards: it runs the firmware, talks over Wi‑Fi (HTTP API), and drives servos, OLED, and audio.

This project uses a **Waveshare ESP32-C3-Zero** (ESP32-C3 chip). Flash and serial go over USB. Radio is **2.4 GHz Wi‑Fi only** (no 5 GHz).

In PlatformIO, `board = esp32-c3-devkitm-1` is a **build target name**, not a different physical module. The real board is still the C3-Zero — same note as in [hardware/components.md](hardware/components.md).

## Happy path

### 1. Parts

Gather electronics from [hardware/components.md](hardware/components.md). Minimum set:

- Waveshare ESP32-C3-Zero
- Adafruit PCA9685 + 5× PowerHD HD-1370A (or equivalent micro servos)
- MAX98357A + 8 Ω / 1 W speaker
- 0.91" 128×32 SSD1306 OLED (I2C)
- Adafruit 5993 USB-C breakout (power + data)
- **5 V / ≥2 A** USB supply (servos need headroom — [hardware/power.md](hardware/power.md))

### 2. Print and mechanical

Printables and CAD: [3d_models/README.md](../3d_models/README.md) (`parts/*.3mf`, source `cad/TinyEngineer.f3d`).

Print settings, fasteners, and a full assembly SOP are **not documented yet** — use the parts table there as the inventory. After print:

- Fit the five servos (head, neck, left/right hand, body) — axes and safe ranges: [robot-movement.md](robot-movement.md)
- Leave the ESP32 ceramic antenna clear of metal / dense plastic ([hardware/components.md](hardware/components.md))

### 3. Wire and power

Canonical connections: [hardware/wiring.md](hardware/wiring.md) and the diagram [wiring/Tiny Engineer.drawio.png](wiring/Tiny%20Engineer.drawio.png). Overview: [hardware/README.md](hardware/README.md).

Before first power-up, run the assembly checks in wiring.md (common GND, PCA9685 **VCC** = 3.3 V vs **V+** = 5 V not shorted, OLED clock on **SCK**, speaker on **SPK+/SPK−** only). Prefer bench bring-up with a strong 5 V supply before seating everything in the printed shell.

### 4. Flash

Firmware is Arduino on [PlatformIO](https://platformio.org/) ([pioarduino](https://github.com/pioarduino/platform-espressif32) / Arduino-ESP32 3.x). Board and baud live in `platformio.ini`.

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) (or the PlatformIO extension). Connect the board over USB (Adafruit 5993 data lines when assembled).

From the project root:

```bash
pio run                 # build
pio run -t upload       # flash firmware + LittleFS
pio device monitor      # serial (115200)
```

After firmware upload, a post-script also uploads **LittleFS** ([`scripts/upload_fs_after_upload.py`](../scripts/upload_fs_after_upload.py)) so WAV assets (`welcome`, `bell`, and friends) land on the board. If animations move but stay silent, run `pio run -t uploadfs` once.

Several serial ports:

```bash
pio device list
pio run -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor --port /dev/cu.usbserial-XXXX
```

### 5. Wi‑Fi setup

First boot (or after factory reset + power-cycle): join setup network `TinyEngineer-XXXX`, open `http://192.168.4.1/config`, enter home Wi‑Fi (**2.4 GHz** only). OLED shows setup steps. Wi‑Fi is not editable from the normal Config page later — factory reset to change it.

### 6. Prove it

Open `http://tiny-engineer.local/` (or the IP on the OLED) for the **web UI**: settings, hardware tests, animations. WiFi credentials stay setup-AP-only (`/config` on `http://192.168.4.1`).

```bash
curl http://tiny-engineer.local/health
curl -X POST "http://tiny-engineer.local/anim?name=ring"
```

If `.local` is slow or fails, use the OLED IP or `curl -4`. Prefer web UI for hardware tests before seating servos hard against stops. When WiFi is not configured, control APIs (`/anim`, `/test/*`) return **503**. Optional access token → `Authorization: Bearer …` on JSON APIs (`GET /auth` stays public) — see [api.md](api.md).

### 7. Optional — Cursor hooks

Robot on the same LAN → [hooks.md](hooks.md). Any IDE / scripts → [integration.md](integration.md). Full HTTP: [api.md](api.md).

## Stuck?

| Symptom | What to try |
| --- | --- |
| Which wires / voltages? | [hardware/wiring.md](hardware/wiring.md), [hardware/pinout.md](hardware/pinout.md) |
| What to print? | [3d_models/README.md](../3d_models/README.md) |
| `.local` slow or fails | OLED IP; `curl -4 http://…` |
| OLED shows join AP / `192.168.4.1` | Wi‑Fi not saved or STA failed — finish setup AP config |
| Welcome / ring silent (servos move) | LittleFS missing WAVs — `pio run -t uploadfs` |
| Hooks never move the robot | Node 18+, hook `timeout` ≥ 30, HTTPS tarball `npx` — see [hooks.md](hooks.md) |
| Servos twitch / board resets on motion | Power budget — [hardware/power.md](hardware/power.md) |
| Other boot / I2C / audio failures | [hardware/testing.md](hardware/testing.md) |
