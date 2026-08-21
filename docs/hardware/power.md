# Power

## Architecture

One common **nominal 5 V** rail feeds the robot.

| Net | Source | Loads |
| --- | --- | --- |
| **+5V** | USB **5V** in the wiring PNG (Adafruit 5993 VBUS) | ESP32 **5V**, PCA9685 **5V**, MAX98357A **Vin**; servos from PCA9685 **V+** |
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
- USB / programming host if that path is also loaded

## USB-C 5993 vs real supply

Adafruit 5993 CC resistors ask the upstream port for **5 V / up to ~1.5 A**. Whether 1.5 A actually arrives depends on the charger/port.

Five servos at stall (~1.3 A @ 4.8 V) plus the rest of the robot leaves **little safety margin** on a 1.5 A USB source.

**TBD — final guaranteed power-supply current.**

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

## Bulk capacitance

If transients appear on **V+** at the PCA9685 (servo start current), add bulk capacitance **near the PCA9685 servo supply (V+)**, not on the 3V3 rail.

Exact value: **TBD — determine experimentally**. Start from typical servo-bus practice (tens to hundreds of µF electrolytic plus local ceramics) and confirm with a scope on V+ during simultaneous servo starts. Do not treat any guessed value as final.

## Power sequencing / dual USB

Two USB-C ports exist (ESP32 for development, 5993 for robot power). Both 5 V nets must not fight.

**TBD — verify on hardware** whether the C3-Zero 5V pad and onboard USB VBUS are directly common, and whether the robot should:

- run from 5993 alone (unplug ESP32 USB after flashing), or
- use a diode/ideal-OR, or
- keep one source only during tests.

Until that is measured: avoid feeding two strong 5 V sources into the same rail without checking for back-feed.

Related: [wiring.md](wiring.md), [components.md](components.md).
