# Power

## Architecture

One common **nominal 5 V** rail feeds the robot.

| Net | Source | Loads |
| --- | --- | --- |
| **+5V** | Adafruit 5993 **VBUS** (USB **5V** in the wiring PNG) | ESP32 **5V**, PCA9685 **5V**, MAX98357A **Vin**; servos from PCA9685 **V+** |
| **3V3** | ESP32 onboard LDO, from the 5 V input | ESP32 core/GPIO, PCA9685 **VCC**, OLED **VCC** |
| **GND** | 5993 GND + ESP32 GND | Everything |

ESP32 internally runs at 3.3 V logic. Servo power **does not** pass through the ESP32 3.3 V regulator. As drawn: USB **5V** → PCA9685 **5V**; PCA9685 **V+** → servo **5V**; PCA9685 **VCC** ← ESP32 **3V3**.

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator.

Waveshare documents the C3-Zero LDO as ME6217C33M5G (hundreds of mA class). That is enough for logic + OLED + PCA9685 digital. It is **not** a servo supply.

## Servo current (worst case)

Datasheet stall currents for HD-1370A:

| Voltage | Per servo stall | × 5 servos |
| --- | --- | --- |
| 4.8 V | ~260 mA | **≈ 1.3 A** |
| 6.0 V | ~320 mA | **≈ 1.6 A** |

Robot rail is nominally **5 V**, so expect something between those two if several servos stall or start together.

This **excludes**:

- ESP32 (Wi-Fi TX peaks)
- OLED
- PCA9685 logic
- MAX98357A + speaker peaks
- USB host current shared with programming when flashing over the same cable

## USB-C 5993 vs real supply

Adafruit 5993 CC resistors ask the upstream port for **5 V / up to ~1.5 A**. Whether 1.5 A actually arrives depends on the charger/port.

Five servos at stall (~1.3 A @ 4.8 V) plus the rest of the robot leaves **little safety margin** on a 1.5 A USB source.

Recommendation:

- **5 V / ≥ 2 A**, with extra margin if several servos can move **while audio plays**.
- 5993 remains a convenient connector; it does not raise the current ceiling above what the source and CC advertisement allow.
- 5993 is **not** USB-PD voltage conversion. Feed it 5 V USB, or bypass it with a dedicated 5 V / ≥2 A PSU on the +5V and GND rails.

## Insufficient-power symptoms

If the rail sags under servo or audio load:

- ESP32 resets / brownouts
- Servo jitter or weak motion
- OLED glitches or blanking
- Audio noise / dropouts
- Unstable I2C (NACKs, PCA9685 or OLED “not found” after motion starts)
- Resets **specifically when multiple servos start moving**

Bring-up test turns all five servos together — that is a power-stress moment.

## Single USB (5993)

The robot uses **one** USB-C connector: the Adafruit 5993.

| Net | 5993 | ESP32 |
| --- | --- | --- |
| +5V | **VBUS** | **5V** pad (and the shared +5V rail) |
| GND | **GND** | **GND** |
| USB D− | **D−** | **GPIO18** (native USB D−) |
| USB D+ | **D+** | **GPIO19** (native USB D+) |

That one cable supplies robot power and programming / serial CDC (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`, monitor 115200).

Leave the ESP32-C3-Zero **onboard USB-C unused** when the robot is assembled so two 5 V sources cannot fight on the same rail. For early bare-module flashing before 5993 data is wired, the onboard port is fine.

Related: [wiring.md](wiring.md), [components.md](components.md), [interfaces.md](interfaces.md).
