# Pinout

Source of truth: [`include/pins.h`](../../include/pins.h). This file must match that header.

## Assigned ESP32-C3-Zero GPIO

| Function | ESP32-C3-Zero GPIO | Constant | Goes to |
| --- | --- | --- | --- |
| I2C SDA | **GPIO0** (diagram **GP0**) | `I2C_SDA` | PCA9685 SDA, OLED SDA |
| I2C SCL | **GPIO1** (diagram **GP1**) | `I2C_SCL` | PCA9685 SCL, Waveshare OLED **SCL** |
| I2S BCLK | **GPIO2** (diagram **GP2**) | `I2S_BCLK` | MAX98357A BCLK |
| I2S LRCLK / WS | **GPIO3** (diagram **GP3**) | `I2S_LRC` | MAX98357A **LRC** |
| I2S DATA OUT | **GPIO4** (diagram **GP4**) | `I2S_DIN` | MAX98357A DIN |
| PCA9685 OE | **GPIO5** (diagram **GP5**) | `PCA9685_OE_PIN` | PCA9685 **OE** (active LOW; `PCA9685_OE_WIRED`) |
| Built-in WS2812 RGB | **GPIO10** | `RGB_LED_PIN` | onboard LED only |

Logic level: **3.3 V**.

## Reserved / unavailable

| GPIO | Status | Reason |
| --- | --- | --- |
| GPIO9 | **Reserved** | BOOT / strapping. On the main control board it is also on **J4** (extension only) — do not pull low at reset (download mode). Do not pick casually for peripherals |
| GPIO10 | **Occupied** | Onboard WS2812 |
| GPIO12–GPIO17 | **Unavailable** | Stacked flash, not brought out |
| GPIO18 | **Reserved** | Native USB D− (main control board USB-C; advanced path: [5993](wiring.md#usb-connector-5993)) |
| GPIO19 | **Reserved** | Native USB D+ (main control board USB-C; advanced path: [5993](wiring.md#usb-connector-5993)) |

## Free / default-function pads

| GPIO | Status | Notes |
| --- | --- | --- |
| GPIO6 | Free | Brought out on **J4** (main control board) |
| GPIO7 | Free | Brought out on **J4** |
| GPIO8 | Free | Brought out on **J4**; strapping — do not pull low at reset |
| GPIO20 | Default UART0 RX | Silkscreen RX; also on **J4**. Free for other use only if USB CDC remains the console (`ARDUINO_USB_CDC_ON_BOOT=1`) |
| GPIO21 | Default UART0 TX | Silkscreen TX; also on **J4**. Same caveat as GPIO20 |

Power pads (not GPIO): **5V**, **GND**, **3V3**.

## Main control board headers

Pin order matches the KiCad board and [`expected-nets.yml`](../../hardware/boards/main-control-board/expected-nets.yml). Board overview: [board README](../../hardware/boards/main-control-board/README.md).

### J2 — OLED (`Conn_OLED`)

| Pin | Net |
| --- | --- |
| 1 | **+3.3V** |
| 2 | **GND** |
| 3 | **I2C SDA** |
| 4 | **I2C SCL** |

### J3 — I2C extension (`Conn_extension`, JST)

Same bus as J2 / PCA9685. **Pin order is not the same as J2** (GND and 3.3 V are swapped).

| Pin | Net |
| --- | --- |
| 1 | **GND** |
| 2 | **+3.3V** |
| 3 | **I2C SDA** |
| 4 | **I2C SCL** |

### J4 — 12-pin extension (`Conn_02x06_Odd_Even`)

| Pin | Net |
| --- | --- |
| 1 | **SERVO_5V** |
| 2 | **GP21** (default UART0 TX) |
| 3 | **GND** |
| 4 | **GP20** (default UART0 RX) |
| 5 | **GND** |
| 6 | **GP9** (strapping / BOOT — do not pull low at reset) |
| 7 | **+3.3V** |
| 8 | **GP8** (strapping — do not pull low at reset) |
| 9 | **I2S BCLK** |
| 10 | **GP7** |
| 11 | **I2S LRC** |
| 12 | **GP6** |

I2S **DIN** is not on J4 (ESP32 → amp only). **SERVO_5V** is the eFuse-protected rail — [power.md](power.md).

## PCA9685 channels (not ESP32 GPIO)

| PCA9685 channel | Firmware | Mechanism |
| --- | --- | --- |
| 0 | `SERVO_HEAD` | Head pitch — [robot-movement.md](../robot-movement.md) |
| 1 | `SERVO_NECK` | Neck yaw |
| 2 | `SERVO_HAND_LEFT` | Left hand |
| 3 | `SERVO_HAND_RIGHT` | Right hand |
| 4 | `SERVO_BODY` | Body / torso |
| 5–15 | unused | available |

## Other constants in `pins.h` (not pins)

| Constant | Value | Meaning |
| --- | --- | --- |
| `SAMPLE_RATE` | 22050 | I2S sample rate |
| `PCA9685_ADDRESS` | `0x40` | I2C |
| `OLED_ADDRESS` | `0x3C` | I2C |
| `OLED_WIDTH` / `OLED_HEIGHT` | 128 / 32 | Display |
| `SERVO_MIN_US` / `SERVO_MAX_US` | 800 / 2200 | Electrical PWM span |
| `SERVO_BOOT_SPEED_DEG_S` | 35 | Boot centering / sleep-pose rate (deg/s) |
| `PCA9685_OE_WIRED` | `true` | GP5 drives PCA9685 OE (active LOW). Set `false` only if OE is hard-tied off-chip |

## Pin allocation rules

New hardware **must not** pick pins ad-hoc.

1. Choose a pad from the **Free** table above, or a documented unused PCA9685 channel.
2. Do not use **Reserved** or **Unavailable** pins.
3. Update **both**:
   - [`include/pins.h`](../../include/pins.h)
   - this `pinout.md`
4. Update [interfaces.md](interfaces.md) and, when the harness or the board plugs change, [main-control-board.md](main-control-board.md) and [wiring.md](wiring.md) in the same change.
5. Stay in the **3.3 V** GPIO domain. Level-shift if a new device is 5 V-only.
6. I2C devices need a unique address on the shared GPIO0/GPIO1 bus.
7. After the edit, grep the repo for old GPIO numbers so comments and tests stay consistent.

Related: [main-control-board.md](main-control-board.md), [interfaces.md](interfaces.md). Advanced harness: [wiring.md](wiring.md).
