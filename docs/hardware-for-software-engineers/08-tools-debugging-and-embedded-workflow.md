# Tools, debugging, and embedded workflow

You don't need an oscilloscope to do useful hardware. You need patience, a multimeter, serial logs, and the discipline to bisect problems like you would in production. This workflow applies to any MCU project; Tiny Engineer is what you'll bench-debug first.

---

## Minimal tool kit

| Tool | Why |
| --- | --- |
| **Multimeter** | DC voltage, continuity beep |
| **USB cable (data)** | Charge-only cables cause "upload failed" existential crisis |
| **5 V / ≥ 2 A supply** | Known-good power |
| **Screwdrivers** | 2 mm screws, horn screws |
| **Soldering iron** (optional) | If your wiring isn't all Dupont |
| **Zip ties / tape** | Strain relief |

Nice later: helping hands, flux, desolder braid. Not day one.

---

## Multimeter — two modes you'll use

**DC voltage (V⎓):** probe between 5V and GND — should be ~5 V under load. Probe 3V3 and GND on ESP32 — ~3.3 V. If 5 V collapses when servos move, supply or wiring.

**Continuity (beep):** power **off** — verify GND nets connect, verify you're not shorting 5V to GND before first power.

You don't need to measure µs PWM pulses. If angles are wrong, suspect firmware limits or horn alignment before reaching for scope rental.

---

## Serial monitor — your stdout

```bash
pio device monitor
```

115200 baud. Boot logs tell you:
- PCA9685 found or not
- OLED found or not
- Wi-Fi state
- Init errors

> **If you've written backend code…** Serial is structured logging before structured logging existed on the device. Read it before guessing.

Upload and monitor details: [getting-started.md § Flash](../getting-started.md#4-flash).

---

## PlatformIO — npm for firmware

From project root:

| Command | Does |
| --- | --- |
| `pio run` | Build firmware |
| `pio run -t upload` | Flash firmware + LittleFS (via post-script) |
| `pio run -t uploadfs` | Filesystem only — if audio silent but servos work |
| `pio device monitor` | Serial |
| `pio device list` | Find port when multiple USB devices |

`platformio.ini` is like `package.json` + Makefile — board target, libraries, upload speed.

**Board name vs physical board:** `esp32-c3-devkitm-1` is the PlatformIO target; physical board is Waveshare C3-Zero. See [Ch. 02](02-microcontrollers-and-esp32.md).

---

## Divide-and-conquer bring-up

Don't assemble everything, power on, and hope. **Bisect** like a failing integration test:

| Stage | Verify |
| --- | --- |
| 1. USB power only | ESP32 boots, serial output, dim green LED |
| 2. + I2C (PCA9685, OLED) | Serial lists devices; PCA9685 required |
| 3. + one servo on ch 0 | `/test/servo?index=0&angle=90` moves it |
| 4. + all servos | Each channel, safe ranges |
| 5. + I2S amp + speaker | Audio test endpoint or animation with sound |
| 6. + mechanical shell | Range of motion, no binding |

Failure table and expected boot sequence: [testing.md](../hardware/testing.md).

---

## HTTP test harness

Once on Wi-Fi, the robot exposes **integration-test-style endpoints**:

- `/health` — alive, CPU temp
- `/test/servo` — single joint
- Web UI — hardware tests without memorizing curl

> **If you've written backend code…** `/test/*` is a manual test harness you hit before E2E. Use it before calling the robot from Cursor hooks.

API shapes: [api.md](../api.md). Agent integration comes **after** hardware works: [integration.md](../integration.md).

---

## Hardware vs software — where to look

| Clue | Likely layer |
| --- | --- |
| Never boots, no serial | Power, USB data wiring, bricked flash |
| Boots, PCA9685 fail | I2C wiring, address, missing chip |
| Boots, resets on motion | Power supply current |
| Boots, one servo dead | That channel's signal or servo |
| Wi-Fi flaky | Antenna clearance, 2.4 GHz only, router |
| Audio silent | I2S wiring, LittleFS not uploaded, SPK+/SPK− |
| Angles wrong but motion OK | Horn alignment, safe range tuning |

When two things changed, revert one. Hardware debugging punishes batch changes like production outages.

---

## Common beginner mistakes (this project)

| Mistake | Consequence |
| --- | --- |
| Swapped SDA/SCL | No I2C devices |
| OLED SCK wired as SPI clock confusion | Same — it's SCL |
| Servos on 3.3 V | Brownout / dead LDO |
| VCC/V+ jumpered on PCA9685 | Logic or servo damage |
| SPK− to GND | Bad audio, possible amp damage |
| Charge-only USB cable | Upload fails |
| Weak 1 A charger | Resets on animation |
| Horn aligned at wrong angle | Binding, weird rest pose |
| Antenna blocked | `.local` doesn't resolve, flaky HTTP |

---

## When things are "done enough"

You're ready for [getting-started.md § Prove it](../getting-started.md#6-prove-it) and agent hooks when:

- `/health` returns OK on Wi-Fi
- Each servo sweeps inside safe range without buzzing
- OLED shows IP (or you're OK without it)
- At least one animation with audio works
- No reset during simultaneous servo + sound

---

**Next:** [How it all fits together](09-how-it-all-fits-together.md)

**Reference:** [hardware/testing.md](../hardware/testing.md) · [getting-started.md](../getting-started.md) · [integration.md](../integration.md)
