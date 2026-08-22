# Hardware testing

Firmware is [`src/main.cpp`](../../src/main.cpp). Boot **inits** hardware and starts HTTP. Demos (OLED, tones, servo sweep) run on demand via REST. Helpers:

- [`src/rgb.cpp`](../../src/rgb.cpp)
- [`src/display/oled.cpp`](../../src/display/oled.cpp)
- [`src/wifi_connect.cpp`](../../src/wifi_connect.cpp)
- [`src/audio.cpp`](../../src/audio.cpp)
- [`src/pca9685_servos.cpp`](../../src/pca9685_servos.cpp)
- [`src/http_server.cpp`](../../src/http_server.cpp), [`src/http/test_handlers.cpp`](../../src/http/test_handlers.cpp)

Constants: [`include/pins.h`](../../include/pins.h). Wi-Fi SSID/password: copy [`include/secrets.h.example`](../../include/secrets.h.example) to `include/secrets.h` (gitignored).

Build/flash: project root README (`pio run`, `pio run -t upload`, serial 115200). Physical board is **Waveshare ESP32-C3-Zero**; PlatformIO env name is `esp32-c3-devkitm-1`.

## What boot covers

| Subsystem | How |
| --- | --- |
| Built-in WS2812 | Green ready (GPIO10) |
| I2C init | `Wire.begin` on GPIO0/GPIO1 |
| OLED | Probe `0x3C`, init (optional) |
| Wi-Fi | STA connect, mDNS `tiny-engineer.local`, OLED shows status + IP |
| PCA9685 | Probe `0x40`, `begin`, 50 Hz |
| MAX98357A / I2S | `I2S.begin` 44.1 kHz 16-bit stereo |
| Servos | Park all channels at 90° |
| HTTP | Port 80 if Wi-Fi connected |
| Success | Dim green RGB during init; then animation LED (see below) |

## Expected boot sequence

1. Serial banner `TINY ENGINEER`
2. `Starting I2C` / `SDA = GP0` / `SCL = GP1`
3. `Checking OLED at 0x3C...` → found or `ERROR: OLED not found` (continues)
4. `WIFI TEST` — OLED `WIFI` / `Connecting...` → `WIFI OK` + IP, or `WIFI FAIL` (continues)
5. `Checking PCA9685 at 0x40...` → **must** succeed
6. `Starting MAX98357A` → `I2S OK`
7. `Centering servos` — all channels → 90°
8. `ROBOT READY` — dim green RGB, OLED `ROBOT READY` + IP (or `WIFI FAIL`)
9. RGB fades to white over 1 s if `welcome` runs (Wi-Fi OK), or fades off if idle
10. If Wi-Fi OK: `HTTP: http://<ip>/`, `HTTP: http://tiny-engineer.local/`, and `/health` URLs on serial

`loop()` pumps the HTTP server and updates animation RGB fades. No audio/OLED/servo/LED demos until a POST.

## Animation RGB

During normal operation the onboard WS2812 tracks the active animation (not boot green):

| Animation | LED |
| --- | --- |
| `typing`, `reading`, `thinking`, `welcome`, `ring` | White |
| `attention`, `error`, `abort` | Red |
| `none` | Off |

State changes fade over **1 s** (see [`docs/api.md`](../api.md#rgb-led)). Trigger via `POST /anim?name=…` or Cursor hooks.

OLED shows matching status strings when the panel is present (`WIFI Connecting...` then IP, `PCA9685 Checking...`, `MAX98357A`).

## HTTP tests

API reference: [`docs/api.md`](../api.md).

Base URL is the board IP or `http://tiny-engineer.local` (2.4 GHz STA). Tests have side effects — use **POST**, not GET.

```bash
# Health
curl http://tiny-engineer.local/health

# Tones 500 / 700 / 1000 Hz
curl -X POST http://tiny-engineer.local/test/audio

# OLED title / HELLO / X in a box
curl -X POST http://tiny-engineer.local/test/screen

# Servos 90 → 105 → 75 → 90 (channels 0–4)
curl -X POST http://tiny-engineer.local/test/movement

# Onboard WS2812 R → G → B → white → off, then back to current animation LED
curl -X POST http://tiny-engineer.local/test/led

# One servo smooth move to angle (~40°/s; index 0–4, angle 0–180)
curl -X POST "http://tiny-engineer.local/test/servo?index=0&angle=90"
```

| Method | Path | Body |
| --- | --- | --- |
| `GET` | `/` | HTML endpoint index |
| `GET` | `/health` | Health JSON (`ok`, `uptime_ms`, `free_heap`, `heap_size`, `wifi`, `oled`) |
| `POST` | `/test/audio` | `{"ok":true,"test":"audio"}` after `runSoundTest()` |
| `POST` | `/test/screen` | `{"ok":true,"test":"screen"}` after `runOledTest()` |
| `POST` | `/test/movement` | `{"ok":true,"test":"movement"}` after `runServoTest()` |
| `POST` | `/test/led` | `{"ok":true,"test":"led"}` after `runRgbTest()` |
| `POST` | `/test/servo?index=&angle=` | `{"ok":true,"test":"servo","index":N,"angle":A}` after `moveServoSmooth()` |

GET on a test path returns `405`. Bad `/test/servo` params return `400`. Unknown path returns `404`. JSON `Content-Type`. Handlers block until the test finishes; the OLED returns to `ROBOT READY` after.

No HTTP if boot Wi-Fi failed (`HTTP: skipped (no wifi)`).

## Failures

| Serial / OLED | Meaning | Check |
| --- | --- | --- |
| OLED `ERROR: OLED not found` then rest of boot runs | Nothing ACK’d at `0x3C` | OLED **VCC=3V3**, GND, SDA=GP0, **SCK**=GP1, common ground, address jumper still 0x3C |
| OLED found but `ERROR: OLED initialization failed` | ACK then `display.begin` failed | Wiring/power glitch, wrong size module, I2C noise |
| OLED `WIFI FAIL` then rest of boot runs | STA connect timed out or auth failed | `include/secrets.h` SSID/password, AP in range, 2.4 GHz (C3 has no 5 GHz) |
| `ERROR: PCA9685 not found` + red RGB + **hang** | Nothing ACK’d at `0x40` | PCA9685 **VCC=3V3** (not V+), GND, SDA/SCL, I2C address pads, +5V not required for the ACK but needed later for motion |
| `ERROR: I2S initialization failed` + red RGB + **hang** | `I2S.begin` failed | GPIO2/3/4 not shorted to 5V/GND; pin constants; USB CDC still alive so you can read the line |
| I2S OK but `POST /test/audio` is silent | Amp or speaker | MAX98357A **Vin**=USB 5V, GND, GP2/3/4 → BCLK/LRC/DIN, speaker on **SPK+ / SPK-** (not on the PNG, not GND) |
| Servos silent / twitch / ESP32 resets during `POST /test/movement` | Power or SIG | **V+** is +5V, SIG on ch 0–4, **common GND**, supply current — see [power.md](power.md) |
| RGB never goes green | GPIO10 LED path | Board is C3-Zero (LED on GPIO10). Do not expect an external NeoPixel |

OLED absence and Wi-Fi failure are **soft** fails. PCA9685 and I2S failures **halt** in `while (true)`.

## Power during the servo test

All five servos move together on `POST /test/movement`. A weak USB port often dies **here**. If the serial port drops exactly when that POST runs: treat as brownout, not a PWM bug. Boot only parks at 90°, which is much lighter.

## What this test does not prove

- Per-servo mechanical limits or channel→joint mapping (**TBD**)
- BLE
- Speaker power rating vs max amp output
- Dual USB-C back-feed behaviour (**TBD**)
- I2C at high speed

Related: [pinout.md](pinout.md), [wiring.md](wiring.md), [servos.md](servos.md).
