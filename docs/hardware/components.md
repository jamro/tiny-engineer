# Hardware inventory

Selected hardware only. Do not substitute parts from this list without updating this file, [pinout.md](pinout.md), and [wiring.md](wiring.md).

Constants cited below match [`include/pins.h`](../../include/pins.h).

## Product links

| Part | Link |
| --- | --- |
| Waveshare ESP32-C3-Zero | https://docs.waveshare.com/ESP32-C3-Zero |
| Adafruit PCA9685 16-channel PWM servo driver | https://www.adafruit.com/product/815 |
| PowerHD HD-1370A | https://www.chd.hk/Product_Detail.aspx?id=30 |
| MAX98357A I2S class-D mono amplifier | https://www.aliexpress.us/item/3256805196806369.html |
| 8 Ω / 1 W mono speaker | https://www.aliexpress.us/item/3256807341987395.html |
| 0.91" 128×32 OLED (SSD1306, I2C) | https://www.raystar-optronics.com/oled-graphic-display-module/oled-i2c-ssd1306.html |
| Adafruit 5993 USB-C breakout | https://www.adafruit.com/product/5993 |

## Waveshare ESP32-C3-Zero

| Field | Value |
| --- | --- |
| Exact model | Waveshare ESP32-C3-Zero (ESP32-C3FH4) |
| Quantity | 1 |
| Purpose | Main controller: I2C, I2S, USB CDC, Wi-Fi, onboard RGB |
| Operating voltage | GPIO **3.3 V**. External power on the **5V** pad: 3.7–6 V (robot uses the +5V rail) |
| Logic | **3.3 V only** — do not drive 5 V into GPIO |
| Radio | Wi-Fi 2.4 GHz (product use). BLE present on silicon but **out of scope** |
| USB | Native USB on GPIO18/19 — reached via Adafruit 5993 D+/D− (onboard USB-C unused when assembled) |
| Flash | 4 MB stacked flash on GPIO12–GPIO17 (those GPIOs are **not exposed**) |
| Onboard RGB | WS2812 on **GPIO10** |
| Firmware notes | Boot: dim green status. Animations: white (typing/reading/thinking/welcome/ring/wakeup), pulsing red (attention/error), solid red (abort), off (idle/sleep). 1 s fade between non-pulse states. Fatal init errors: solid dim red + hang. See [`docs/api.md`](../../api.md#rgb-led) |
| BOOT | Button / strapping on **GPIO9** |
| RESET | Separate RESET button (CHIP_EN), not a GPIO |
| UART0 silkscreen | TX = GPIO21, RX = GPIO20 |
| USB D− / D+ | GPIO18 / GPIO19 (native USB) |
| 3V3 regulator | Onboard LDO (Waveshare: ME6217C33M5G). For **logic loads only** |
| Important pins | See [pinout.md](pinout.md) |
| Limits | Do not power servos from **3V3**. Do not cover the ceramic antenna with metal, PCB, or plastic. |
| Firmware note | PlatformIO `board = esp32-c3-devkitm-1` is a **build target**, not the physical module. Physical board is C3-Zero. |

## Adafruit PCA9685 16-channel PWM servo driver

| Field | Value |
| --- | --- |
| Exact model | Adafruit 16-Channel 12-bit PWM/Servo Driver (PCA9685) |
| Quantity | 1 |
| Purpose | Independent 50 Hz servo PWM so ESP32 timing is not the PWM source |
| Operating voltage | **VCC** (logic): 3.3 V from ESP32 **3V3**. **V+** (servos): +5V rail |
| Interface | I2C |
| Address | `0x40` (`PCA9685_ADDRESS` in firmware). A0–A5 pads can change this |
| PWM | 12-bit, firmware `setPWMFreq(50)` |
| Important pins | As drawn: **5V** (servo-rail in), **V+** (to servo 5V), **VCC** (3V3 logic), **GND**, **SDA**, **SCL**, **PWM**. OE not drawn |
| Channels used | 0–4 (five servos). 5–15 unused |
| Limits | **V+** and **VCC** must stay separate. Do not jumper servo power onto logic VCC on this robot |
| Firmware note | Detection at `0x40` is a **hard fail** in the bring-up test |

## PowerHD HD-1370A

| Field | Value |
| --- | --- |
| Exact model | PowerHD HD-1370A analog micro servo |
| Quantity | 5 |
| Purpose | Robot joints (Head / Neck / L hand / R hand / Body — [robot-movement.md](../robot-movement.md)) |
| Operating voltage | 4.8–6.0 V (robot: **+5V** via PCA9685 **V+**) |
| Interface | Analog PWM, ~50 Hz. Signal is 3.3 V from PCA9685 — acceptable for this servo class |
| Pulse | Neutral ~1500 µs. Declared range ~**800–2200 µs** (`SERVO_MIN_US` / `SERVO_MAX_US`) |
| Current (datasheet stall) | ~260 mA @ 4.8 V, ~320 mA @ 6.0 V |
| No-load current | ~110 mA @ 4.8 V, ~120 mA @ 6.0 V |
| Idle current | ~4 mA @ 4.8 V, ~5 mA @ 6.0 V |
| Torque / speed | 0.4 kg·cm / 0.12 s/60° @ 4.8 V; 0.6 kg·cm / 0.10 s/60° @ 6.0 V |
| Size / mass | ~20.2 × 8.5 × 17.6 mm, ~3.7 g |
| Important pins | SIG, +5V, GND (JR-style 3-wire) |
| Limits | Manufacturer sheet also lists **~130°** travel over 800–2200 µs, while many retailers say 0–180°. Treat 0–180° as **nominal**, not a mechanical guarantee. |

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator.

Firmware uses **per-servo safe ranges** in [`include/servos.h`](../../include/servos.h) (`SERVO_SPECS`). Tune after assembly if needed. Do not command full 0–180° / 800–2200 µs on the assembled robot. See [servos.md](servos.md).

## MAX98357A I2S class-D mono amplifier

| Field | Value |
| --- | --- |
| Exact model | MAX98357A breakout (I2S in, speaker out) |
| Quantity | 1 |
| Purpose | Digital audio → mono speaker. ESP32 never drives the speaker as GPIO PWM |
| Operating voltage | **Vin** 2.5–5.5 V. Drawing: USB **5V** → **Vin** |
| I2S logic | 3.3 V from ESP32 (compatible) |
| Interface | I2S: **BCLK**, **WS/LRC**, **DIN**. No MCLK |
| Sample rates | Chip: 8–96 kHz. Firmware: **44100 Hz**, 16-bit, stereo slot mode |
| Output power (typical) | ~1.8 W into 8 Ω @ 5 V / 10% THD (datasheet-class figure) |
| Important pins | Drawn: **Vin**, **GND**, **BCLK**, **LRC**, **DIN**. **SPK+** / **SPK-** not drawn. **GAIN** / **SD** not wired (breakout defaults: GAIN floating ≈ 9 dB, SD pulled up ≈ left channel) |
| Limits | Output is **BTL**. Do not connect SPK- to GND. Do not feed SPK+/SPK- into another amplifier. Do not drive the speaker from ESP32 GPIO |

> [!WARNING]
> MAX98357A SPK- is not ground.

## Speaker

| Field | Value |
| --- | --- |
| Exact model | 8 Ω / 1 W mono speaker (AliExpress listing above) |
| Quantity | 1 |
| Purpose | Acoustic output |
| Operating voltage | Not a voltage-rail device — driven by MAX98357A BTL |
| Interface | Two terminals → **SPK+** and **SPK-** |
| Limits | 1 W rated. Amp can theoretically exceed 1 W into 8 Ω at 5 V; keep firmware volume moderate. Never connect either terminal to **GND** |

Speaker wiring is **not** drawn on `docs/wiring/Tiny Engineer.drawio`. Electrical rule is still: speaker ↔ SPK+/SPK- only.

## 0.91" 128×32 OLED (SSD1306, I2C)

| Field | Value |
| --- | --- |
| Exact model | Generic 0.91" 128×32 SSD1306 I2C OLED |
| Quantity | 1 |
| Purpose | Status / test UI |
| Operating voltage | Module **VCC** from ESP32 **3V3** (confirmed in [`docs/wiring`](../wiring/Tiny%20Engineer.drawio.png)) |
| Interface | I2C on the shared bus |
| Address | **`0x3C`** (`OLED_ADDRESS` in firmware). Alternate 0x3D exists on some modules if the ADDR jumper is changed — not used here |
| Resolution | 128×32 (`OLED_WIDTH` / `OLED_HEIGHT`) |
| Important pins | As drawn: **VCC**, **GND**, **SDA**, **SCK** (that **SCK** pad is I2C SCL) |
| Firmware notes | `SSD1306_SWITCHCAPVCC`; display rotated **180°** (`setRotation(2)`). Missing OLED is **non-fatal** in the bring-up test |
| Limits | 3.3 V logic. Do not hang this module on the servo +5V rail unless a specific 5 V-tolerant module is verified (**current wiring is 3V3**) |

## Adafruit 5993 USB-C breakout

| Field | Value |
| --- | --- |
| Exact model | Adafruit 5993 — USB Type C Vertical Breakout, downstream connection |
| Quantity | 1 |
| Purpose | Sole robot USB-C: **+5V power** and **programming / CDC** |
| Operating voltage | USB **VBUS** = 5 V |
| Electrical | Two 5.1 kΩ CC resistors request 5 V and **up to ~1.5 A**. Actual current is whatever the **upstream source** can give |
| Interface | **VBUS**, **GND**, **D+**, **D−**. Wire D−→ESP32 GPIO18, D+→ESP32 GPIO19 |
| Important pins | Drawing may show USB **5V** / **GND** only; also wire **D+/D−** to native USB |
| Limits | **Not** a USB-PD voltage converter. It does not step 9 V/12 V/20 V down to 5 V. Prefer a **5 V / ≥2 A** host — see [power.md](power.md) |

Prefer a final 5 V supply of **at least ~2 A** with margin if several servos move while audio plays.

## Mechanical / RF notes for firmware and CAD

- Keep metal and dense plastic **off** the ESP32 ceramic antenna area.
- Servo horns and end stops: start from `SERVO_SPECS` in [`include/servos.h`](../../include/servos.h); tune after installation if needed. Bench-only test band remains 75–105°.
- GPIO18/19 are native USB via the 5993 data wires. Do not reassign them while using USB CDC for flash/serial.
