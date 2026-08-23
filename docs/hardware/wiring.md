# Wiring

Source of truth for *what is connected to what*: [`docs/wiring/Tiny Engineer.drawio.png`](../wiring/Tiny%20Engineer.drawio.png) ([`.drawio`](../wiring/Tiny%20Engineer.drawio)).

Firmware GPIO numbers: [`include/pins.h`](../../include/pins.h) / [pinout.md](pinout.md). Diagram labels pads **GP0**…**GP21**; those are the same pins as GPIO0…GPIO21.

Diagram colour: **red** = 5 V, **blue** = GND, thin wires = signals.

## Pin names as drawn

| Diagram label | Same net / meaning |
| --- | --- |
| USB **5V** | +5V rail (Adafruit 5993 **VBUS**) |
| USB **GND** | Common **GND** |
| ESP32 **GP0** | GPIO0, I2C **SDA** |
| ESP32 **GP1** | GPIO1, I2C **SCL** |
| ESP32 **GP2** | GPIO2, I2S **BCLK** |
| ESP32 **GP3** | GPIO3, I2S **WS/LRC** |
| ESP32 **GP4** | GPIO4, I2S **DIN** |
| PCA9685 **5V** | Servo-power *input* from USB 5V |
| PCA9685 **V+** | Servo-power *output* to servo 5V |
| PCA9685 **VCC** | Logic **3V3** |
| OLED **SCK** | I2C **SCL** |
| MAX98357A **Vin** | Amp **+5V** (**VIN**) |
| MAX98357A **LRC** | **WS/LRC** |

On Adafruit PCA9685, header/terminal **V+** is normally the same servo rail as the diagram’s PCA9685 **5V** pad. Do **not** treat PCA9685 **5V** as **VCC**.

## Connection table (as drawn)

| From | Pin | To | Pin | Purpose |
| --- | --- | --- | --- | --- |
| USB | 5V | ESP32-C3-Zero | 5V | Board 5 V in |
| USB | 5V | PCA9685 | 5V | Servo-rail in |
| USB | 5V | MAX98357A | Vin | Amp power |
| USB | GND | ESP32-C3-Zero | GND | Common ground |
| USB | GND | PCA9685 | GND (logic/header side) | Common ground |
| USB | GND | OLED | GND | Common ground |
| USB | GND | MAX98357A | GND | Common ground |
| ESP32-C3-Zero | 3V3 | PCA9685 | VCC | PCA9685 logic |
| ESP32-C3-Zero | 3V3 | OLED | VCC | OLED power |
| ESP32-C3-Zero | GP0 | PCA9685 | SDA | I2C data |
| ESP32-C3-Zero | GP0 | OLED | SDA | I2C data (shared) |
| ESP32-C3-Zero | GP1 | PCA9685 | SCL | I2C clock |
| ESP32-C3-Zero | GP1 | OLED | SCK | I2C clock (shared; module silkscreen SCK) |
| ESP32-C3-Zero | GP2 | MAX98357A | BCLK | I2S bit clock |
| ESP32-C3-Zero | GP3 | MAX98357A | LRC | I2S word select |
| ESP32-C3-Zero | GP4 | MAX98357A | DIN | I2S data |
| ESP32-C3-Zero | GP5 | PCA9685 | OE | Optional output enable (see [interfaces.md](interfaces.md)) |
| PCA9685 | PWM | Servo | SIG | Servo PWM |
| PCA9685 | V+ | Servo | 5V | Servo power |
| PCA9685 | GND (servo side) | Servo | GND | Servo ground |

**All modules must share a common ground.** USB GND is the star in the drawing (ESP32, PCA9685 header, OLED, MAX98357A). Servo GND returns through PCA9685 servo-side GND, which is the same ground net.

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator. Diagram: servo **5V** ← PCA9685 **V+**; PCA9685 **VCC** ← ESP32 **3V3** only.

## Power (as drawn)

```
USB 5V
  → ESP32 5V
  → PCA9685 5V
  → MAX98357A Vin

PCA9685 V+
  → Servo 5V
```

```
ESP32 3V3
  → PCA9685 VCC
  → OLED VCC
```

```
USB GND
  → ESP32 GND
  → PCA9685 GND (header)
  → OLED GND
  → MAX98357A GND

PCA9685 GND (servo header)
  → Servo GND
```

USB block in the drawing is the robot **power** input (Adafruit 5993). It is not the ESP32’s own USB-C programming port.

## I2C (as drawn)

Shared bus:

- ESP32 **GP0** → PCA9685 **SDA** and OLED **SDA**
- ESP32 **GP1** → PCA9685 **SCL** and OLED **SCK**

Addresses (firmware, not on the drawing): PCA9685 `0x40`, OLED `0x3C`. See [interfaces.md](interfaces.md).

## I2S (as drawn)

- ESP32 **GP2** → MAX98357A **BCLK**
- ESP32 **GP3** → MAX98357A **LRC**
- ESP32 **GP4** → MAX98357A **DIN**
- USB **5V** → MAX98357A **Vin**
- USB **GND** → MAX98357A **GND**

Drawn MAX98357A pads: **Vin**, **GND**, **BCLK**, **LRC**, **DIN** only.

## PCA9685 → servo (as drawn)

One generic **Servo** block:

| Servo | PCA9685 |
| --- | --- |
| SIG | PWM |
| 5V | V+ |
| GND | GND |

Firmware drives **channels 0–4** the same way (`SERVO_CHANNELS`). The drawing does not number the PWM header. Channel-to-mechanism mapping: **TBD** — [servos.md](servos.md).

## Pads drawn with no wires

ESP32-C3-Zero, unused in the PNG:

**GP6, GP7, GP8, GP9, GP10, GP18, GP19, GP20, GP21**

GP5 = optional PCA9685 OE (not in PNG). GP9 = BOOT, GP10 = onboard WS2812, GP18/GP19 = native USB. See [pinout.md](pinout.md).

PCA9685 **OE** optional on GP5 — not drawn on base PNG. MAX98357A **GAIN** / **SD** / **SPK+** / **SPK-** not drawn.

## Not on the drawing

Still part of the selected hardware; do not invent extra ESP32 GPIO for them.

| Item | Rule |
| --- | --- |
| Speaker | Terminals → MAX98357A **SPK+** and **SPK-** only |
| 4 extra HD-1370A | Same 3-wire pattern as the drawn servo, on PCA9685 PWM 0–4 |
| Adafruit 5993 D+/D−/CC/SBU | Unused; drawing uses USB **5V** and **GND** only |

> [!WARNING]
> MAX98357A output is BTL. **SPK- is not ground.** Never tie SPK- or SPK+ to GND. Never drive the speaker from ESP32 GPIO.

## USB connectors (two different things)

| Connector | On the drawing? | Role |
| --- | --- | --- |
| Block labelled **USB** | Yes (5V + GND) | Robot +5V in (Adafruit 5993) |
| ESP32-C3-Zero USB-C | No | Flash + serial CDC |

See [interfaces.md](interfaces.md).

## Assembly checks

1. Every **GND** in the PNG is one net (USB, ESP32, PCA9685 both sides, OLED, MAX98357A, servo).
2. PCA9685 **VCC** ≈ 3.3 V, PCA9685 **5V** / **V+** ≈ 5 V, those two nets **not** shorted.
3. OLED clock wire lands on the pad labelled **SCK**.
4. I2S is GP2/GP3/GP4 → BCLK/LRC/DIN, not swapped with I2C.
5. Speaker (if fitted) is only SPK+/SPK-, which are **not** in the PNG.
