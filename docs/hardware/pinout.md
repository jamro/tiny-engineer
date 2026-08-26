# Pinout

Source of truth: [`include/pins.h`](../../include/pins.h). This file must match that header.

Verified against firmware (2026-08-21). No discrepancy vs the tested assignment.

## Assigned ESP32-C3-Zero GPIO

| Function | ESP32-C3-Zero GPIO | Constant | Goes to |
| --- | --- | --- | --- |
| I2C SDA | **GPIO0** (diagram **GP0**) | `I2C_SDA` | PCA9685 SDA, OLED SDA |
| I2C SCL | **GPIO1** (diagram **GP1**) | `I2C_SCL` | PCA9685 SCL, OLED **SCK** |
| I2S BCLK | **GPIO2** (diagram **GP2**) | `I2S_BCLK` | MAX98357A BCLK |
| I2S LRCLK / WS | **GPIO3** (diagram **GP3**) | `I2S_LRC` | MAX98357A **LRC** |
| I2S DATA OUT | **GPIO4** (diagram **GP4**) | `I2S_DIN` | MAX98357A DIN |
| PCA9685 OE (optional) | **GPIO5** (diagram **GP5**) | `PCA9685_OE_PIN` | PCA9685 **OE** when `PCA9685_OE_WIRED` |
| Built-in WS2812 RGB | **GPIO10** | `RGB_LED_PIN` | onboard LED only |

Logic level: **3.3 V**.

## Reserved / unavailable

| GPIO | Status | Reason |
| --- | --- | --- |
| GPIO9 | **Reserved** | BOOT button / strapping. Do not pick casually for peripherals |
| GPIO10 | **Occupied** | Onboard WS2812 |
| GPIO12–GPIO17 | **Unavailable** | Stacked flash, not brought out |
| GPIO18 | **Reserved** | Native USB D− (via Adafruit 5993 D−) |
| GPIO19 | **Reserved** | Native USB D+ (via Adafruit 5993 D+) |

## Free / default-function pads

| GPIO | Status | Notes |
| --- | --- | --- |
| GPIO5 | **Optional** | PCA9685 **OE** when wired; set `PCA9685_OE_WIRED = true` in `pins.h` |
| GPIO6 | Free | Unused |
| GPIO7 | Free | Unused |
| GPIO8 | Free | Unused |
| GPIO20 | Default UART0 RX | Silkscreen RX. Free for other use only if USB CDC remains the console (`ARDUINO_USB_CDC_ON_BOOT=1`) |
| GPIO21 | Default UART0 TX | Silkscreen TX. Same caveat as GPIO20 |

Power pads (not GPIO): **5V**, **GND**, **3V3**.

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
| `SAMPLE_RATE` | 44100 | I2S sample rate |
| `PCA9685_ADDRESS` | `0x40` | I2C |
| `OLED_ADDRESS` | `0x3C` | I2C |
| `OLED_WIDTH` / `OLED_HEIGHT` | 128 / 32 | Display |
| `SERVO_MIN_US` / `SERVO_MAX_US` | 800 / 2200 | Electrical PWM span |
| `SERVO_BOOT_SPEED_DEG_S` | 35 | Boot centering / sleep-pose rate (deg/s) |
| `PCA9685_OE_WIRED` | `false` | Set `true` after wiring GP5 → PCA9685 OE |
| `SERVO_LOW` / `SERVO_CENTER` / `SERVO_HIGH` | 75 / 90 / 105 | Hardware-test angles |

## Pin allocation rules

New hardware **must not** pick pins ad-hoc.

1. Choose a pad from the **Free** table above, or a documented unused PCA9685 channel.
2. Do not use **Reserved** or **Unavailable** pins.
3. Update **both**:
   - [`include/pins.h`](../../include/pins.h)
   - this `pinout.md`
4. Update [wiring.md](wiring.md) and [interfaces.md](interfaces.md) in the same change.
5. Stay in the **3.3 V** GPIO domain. Level-shift if a new device is 5 V-only.
6. I2C devices need a unique address on the shared GPIO0/GPIO1 bus.
7. After the edit, grep the repo for old GPIO numbers so comments and tests stay consistent.

Related: [wiring.md](wiring.md), [interfaces.md](interfaces.md).
