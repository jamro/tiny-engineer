# Hardware testing

Firmware is [`src/main.cpp`](../../src/main.cpp). Boot **inits** hardware and starts HTTP. Demos (OLED, tones, servo sweep) run on demand via REST. Helpers:

- [`src/hardware/rgb.cpp`](../../src/hardware/rgb.cpp)
- [`src/display/oled.cpp`](../../src/display/oled.cpp) (init/boot/test: `oled_init.cpp`, `oled_boot.cpp`, `oled_test.cpp`)
- [`src/network/wifi_connect.cpp`](../../src/network/wifi_connect.cpp)
- [`src/audio/audio.cpp`](../../src/audio/audio.cpp)
- [`src/hardware/pca9685_servos.cpp`](../../src/hardware/pca9685_servos.cpp)
- [`src/http/http_server.cpp`](../../src/http/http_server.cpp), [`src/http/test_handlers.cpp`](../../src/http/test_handlers.cpp)

Constants: [`include/pins.h`](../../include/pins.h). Wi-Fi credentials are saved in NVS and configured only in setup AP mode (first boot, or after factory reset + power-cycle).

Build/flash: project root README (`pio run`, `pio run -t upload`, serial 115200). Physical board is **Waveshare ESP32-C3-Zero**; PlatformIO env name is `esp32-c3-devkitm-1`.

## What boot covers

| Subsystem | How |
| --- | --- |
| Built-in WS2812 | Green ready (GPIO10) |
| I2C init | `Wire.begin` on GPIO0/GPIO1 |
| PCA9685 | Probe `0x40` early; park neutral; optional OE on GP5 |
| OLED | Probe `0x3C`, init (optional) |
| Wi-Fi | STA connect from saved NVS credentials, or setup AP `TinyEngineer-XXXX` when unset/failed; mDNS `{hostname}.local` after STA connect |
| MAX98357A / I2S | `I2S.begin` 44.1 kHz 16-bit stereo |
| Servos | Smooth move to mid (or sleep pose) at 35°/s |
| HTTP | Port 80 if Wi-Fi connected |
| Success | Dim green RGB during init; then animation LED (see below) |

## Expected boot sequence

1. Serial banner `TINY ENGINEER`
2. `Starting I2C` / `SDA = GP0` / `SCL = GP1`
3. `Checking PCA9685 at 0x40...` → **must** succeed; all channels parked at mid
4. `Checking OLED at 0x3C...` → found or `ERROR: OLED not found` (continues)
5. Settings load from NVS (`loading` = `progress` or `sleep_inertia`)
6. **Progress loading (default):** OLED progress steps (Display → WiFi → Servos → Audio → Storage → Ready), then large full-width IP (or `No IP`) for 3 s, then idle eyes
7. **Sleep inertia loading:** closed eyes during init; smooth move to sleep pose if `welcome` is on; slow eye open + blinks (~5.5 s). Head/neck wave only if `welcome` is on; otherwise eyes only
8. `WIFI SETUP` on serial — connect OK + IP, open setup AP, or saved credentials failed (setup AP for reconfiguration)
9. `Starting MAX98357A` → `I2S OK`
10. `Centering servos` — smooth move to per-channel mid (progress path only; skipped if sleep pose already applied)
11. `ROBOT READY`
12. RGB fades to white over 1 s if `welcome` runs (Wi-Fi OK and setting enabled), or fades off if idle
13. If Wi-Fi OK: `HTTP: http://<ip>/`, `HTTP: http://tiny-engineer.local/`, and `/health` URLs on serial
14. If setup AP active: `HTTP setup: http://192.168.4.1/config` on serial; OLED rotates setup instructions

`loop()` pumps the HTTP server and updates animation RGB fades. No audio/OLED/servo/LED demos until a POST.

## Animation RGB

During normal operation the onboard WS2812 tracks the active animation (not boot green):

| Animation | LED |
| --- | --- |
| `typing`, `reading`, `thinking`, `welcome`, `ring`, `wakeup` | White |
| `attention`, `error`, `dead` | Pulsing red (10%↔100%, 1.5 s cycle) |
| `abort` | Solid red |
| `none`, `sleep` | Off |

State changes fade over **1 s** (see [`docs/api.md`](../api.md#rgb-led)). Trigger via `POST /anim?name=…` or Cursor hooks.

OLED shows matching status strings when the panel is present (progress loading: WiFi step labels; sleep inertia: eyes only).

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
| `GET` | `/auth` | Auth status (`ok`, `required`) — always public |
| `GET` | `/health` | Health JSON (`ok`, `uptime_ms`, `free_heap`, `heap_size`, `cpu_temp_c`, `wifi`, `oled`) |
| `GET` | `/settings` | Persistent settings (`sleep_timeout`, `hostname`, `volume`, `welcome`, `serial_log`, `continuous_timeout`, `loading`, `access_token_set`, `wifi_configured`, `wifi_ssid`, `wifi_password_set`) |
| `POST` | `/settings?...&wifi_ssid=&wifi_password=` | Update NVS settings; WiFi params setup-AP-only and tested before save; `reboot_required` if hostname changed |
| `POST` | `/settings/reset` | Factory reset all settings to defaults (clears WiFi); power-cycle to reopen setup AP |
| `POST` | `/test/audio` | `{"ok":true,"test":"audio"}` after `runSoundTest()` |
| `POST` | `/test/screen` | `{"ok":true,"test":"screen"}` after `runOledTest()` |
| `POST` | `/test/movement` | `{"ok":true,"test":"movement"}` after `runServoTest()` |
| `POST` | `/test/led` | `{"ok":true,"test":"led"}` after `runRgbTest()` |
| `POST` | `/test/servo?index=&angle=` | `{"ok":true,"test":"servo","index":N,"angle":A}` after `moveServoSmooth()` |

GET on a test path returns `405`. Bad `/test/servo` or `/settings` params return `400`. Missing/wrong Bearer when auth enabled returns `401`. Control APIs return `503` when WiFi credentials are not saved. Unknown path returns `404`. JSON `Content-Type`. Handlers block until the test finishes; the OLED returns to `ROBOT READY` after.

HTTP runs on STA when connected, or on setup AP at `192.168.4.1` during provisioning.

## Failures

| Serial / OLED | Meaning | Check |
| --- | --- | --- |
| OLED `ERROR: OLED not found` then rest of boot runs | Nothing ACK’d at `0x3C` | OLED **VCC=3V3**, GND, SDA=GP0, **SCK**=GP1, common ground, address jumper still 0x3C |
| OLED found but `ERROR: OLED initialization failed` | ACK then `display.begin` failed | Wiring/power glitch, wrong size module, I2C noise |
| OLED shows `Join this WiFi` / AP name, then `Then open` / `192.168.4.1` | Setup AP mode active | Connect to the shown AP, open `http://192.168.4.1/config`, enter home WiFi credentials |
| OLED `WiFi failed` then setup AP | Saved STA credentials failed | Join setup AP, open `http://192.168.4.1/config`, enter home WiFi again |
| `ERROR: PCA9685 not found` + red RGB + **hang** | Nothing ACK’d at `0x40` | PCA9685 **VCC=3V3** (not V+), GND, SDA/SCL, I2C address pads, +5V not required for the ACK but needed later for motion |
| `ERROR: I2S initialization failed` + red RGB + **hang** | `I2S.begin` failed | GPIO2/3/4 not shorted to 5V/GND; pin constants; USB CDC still alive so you can read the line |
| I2S OK but `POST /test/audio` is silent | Amp or speaker | MAX98357A **Vin**=USB 5V, GND, GP2/3/4 → BCLK/LRC/DIN, speaker on **SPK+ / SPK-** (not on the PNG, not GND) |
| Servos silent / twitch / ESP32 resets during `POST /test/movement` | Power or SIG | **V+** is +5V, SIG on ch 0–4, **common GND**, supply current — see [power.md](power.md) |
| RGB never goes green | GPIO10 LED path | Board is C3-Zero (LED on GPIO10). Do not expect an external NeoPixel |

OLED absence and Wi-Fi failure are **soft** fails. PCA9685 and I2S failures **halt** in `while (true)`.

## Power during the servo test

All five servos move together on `POST /test/movement`. A weak USB port often dies **here**. If the serial port drops exactly when that POST runs: treat as brownout, not a PWM bug. Boot only parks at 90°, which is much lighter.

## What this test does not prove

- That tuned mechanical limits match a particular physical build (start from `SERVO_SPECS`; adjust after assembly)
- BLE (out of scope)
- Speaker power rating vs max amp output
- I2C at high speed

Related: [pinout.md](pinout.md), [wiring.md](wiring.md), [servos.md](servos.md).
