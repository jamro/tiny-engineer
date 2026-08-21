# Hardware testing

Bring-up firmware is [`src/main.cpp`](../../src/main.cpp). Helpers:

- [`src/rgb.cpp`](../../src/rgb.cpp)
- [`src/oled.cpp`](../../src/oled.cpp)
- [`src/audio.cpp`](../../src/audio.cpp)
- [`src/pca9685_servos.cpp`](../../src/pca9685_servos.cpp)

Constants: [`include/pins.h`](../../include/pins.h).

Build/flash: project root README (`pio run`, `pio run -t upload`, serial 115200). Physical board is **Waveshare ESP32-C3-Zero**; PlatformIO env name is `esp32-c3-devkitm-1`.

## What it covers

| Subsystem | How |
| --- | --- |
| Built-in WS2812 | R → G → B → white → off |
| I2C init | `Wire.begin` on GPIO0/GPIO1 |
| OLED | Probe `0x3C`, init, text/graphics (optional) |
| PCA9685 | Probe `0x40`, `begin`, 50 Hz |
| MAX98357A / I2S | `I2S.begin` 44.1 kHz 16-bit stereo, then tones |
| Servos | Channels 0–4, 90 → 105 → 75 → 90 |
| Success | Green RGB, OLED `ROBOT READY` / `All tests OK` |

## Expected sequence

1. Serial banner `ROBOT HARDWARE TEST`
2. RGB test → `RGB OK`
3. `Starting I2C` / `SDA = GP0` / `SCL = GP1`
4. `Checking OLED at 0x3C...` → found or `ERROR: OLED not found` (continues)
5. If OLED present: `OLED TEST` (title, `HELLO`, X in a box) → `OLED OK`
6. `Checking PCA9685 at 0x40...` → **must** succeed
7. `Starting MAX98357A` → `I2S OK` → `SOUND TEST` 500 / 700 / 1000 Hz → `Sound OK`
8. `SERVO TEST - CHANNELS 0-4` (four moves) → `All 5 servos test finished`
9. `ALL TESTS FINISHED` — RGB green, OLED `ROBOT READY`

`loop()` only delays. No continuous animation after success.

OLED shows matching status strings when the panel is present (`PCA9685 Checking...`, `MAX98357A`, servo progress bar `SERVO TEST x5`).

## Failures

| Serial / OLED | Meaning | Check |
| --- | --- | --- |
| OLED `ERROR: OLED not found` then rest of tests run | Nothing ACK’d at `0x3C` | OLED **VCC=3V3**, GND, SDA=GP0, **SCK**=GP1, common ground, address jumper still 0x3C |
| OLED found but `ERROR: OLED initialization failed` | ACK then `display.begin` failed | Wiring/power glitch, wrong size module, I2C noise |
| `ERROR: PCA9685 not found` + red RGB + **hang** | Nothing ACK’d at `0x40` | PCA9685 **VCC=3V3** (not V+), GND, SDA/SCL, I2C address pads, +5V not required for the ACK but needed later for motion |
| `ERROR: I2S initialization failed` + red RGB + **hang** | `I2S.begin` failed | GPIO2/3/4 not shorted to 5V/GND; pin constants; USB CDC still alive so you can read the line |
| I2S OK but no sound | Amp or speaker | MAX98357A **Vin**=USB 5V, GND, GP2/3/4 → BCLK/LRC/DIN, speaker on **SPK+ / SPK-** (not on the PNG, not GND) |
| Servos silent / twitch / ESP32 resets during step 2–4 | Power or SIG | **V+** is +5V, SIG on ch 0–4, **common GND**, supply current — see [power.md](power.md) |
| RGB never changes | GPIO10 LED path | Board is C3-Zero (LED on GPIO10). Do not expect an external NeoPixel |

OLED absence is the only **soft** fail. PCA9685 and I2S failures **halt** in `while (true)`.

## Power during the servo step

All five servos move together. A weak USB port often dies **here**, not during RGB/OLED. If the serial port drops exactly when servos start: treat as brownout, not a PWM bug.

## What this test does not prove

- Per-servo mechanical limits or channel→joint mapping (**TBD**)
- Wi-Fi / BLE
- Speaker power rating vs max amp output
- Dual USB-C back-feed behaviour (**TBD**)
- I2C at high speed

Related: [pinout.md](pinout.md), [wiring.md](wiring.md), [servos.md](servos.md).
