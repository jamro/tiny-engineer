# Power

## Architecture

One common **nominal 5 V** rail feeds the robot.

| Net | Source | Loads |
| --- | --- | --- |
| **+5V** | Main control board USB-C **VBUS** | ESP32 **5V**, PCA9685 **5V**, MAX98357A **Vin**; servos from PCA9685 **V+** |
| **3V3** | ESP32 onboard LDO, from the 5 V input | ESP32 core/GPIO, PCA9685 **VCC**, OLED **VCC** |
| **GND** | Board USB-C GND + ESP32 GND | Everything |

ESP32 internally runs at 3.3 V logic. Servo power **does not** pass through the ESP32 3.3 V regulator. USB **5V** → PCA9685 **5V**; PCA9685 **V+** → servo **5V**; PCA9685 **VCC** ← ESP32 **3V3**. The advanced breakout harness draws the same nets — [wiring.md](wiring.md).

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator.

Waveshare documents the C3-Zero LDO as ME6217C33M5G (hundreds of mA class). That is enough for logic + OLED + PCA9685 digital. It is **not** a servo supply.

## Servo current (worst case)

Datasheet stall currents for PowerHD HD-1370A:

| Voltage | Per servo stall | × 5 servos |
| --- | --- | --- |
| 4.8 V | ~260 mA | **≈ 1.3 A** |
| 6.0 V | ~320 mA | **≈ 1.6 A** |

Robot rail is nominally **5 V**, so expect something between those two if several HD-1370A servos stall or start together.

**Tower Pro SG90** (recommended) is larger and typically stalls harder. Same **5 V / ≥ 2 A** floor; give more margin if several SG90s move while audio plays. Do not treat the HD-1370A milliamp table as an SG90 budget.

This **excludes**:

- ESP32 (Wi-Fi TX peaks)
- OLED
- PCA9685 logic
- MAX98357A + speaker peaks
- USB host current shared with programming when flashing over the same cable

## USB supply

Power and data enter on the **main control board USB-C** (one cable). Nets: VBUS → +5V, GND, D− → GPIO18, D+ → GPIO19. Details: [interfaces.md](interfaces.md).

Two 5.1 kΩ CC resistors on the board mark it as a USB device. The charger decides how much current arrives. Use a **5 V / ≥ 2 A** source, with extra margin if several servos move **while audio plays**. The connector is **not** USB-PD voltage conversion. Feed it 5 V USB.

Five HD-1370A servos at stall (~1.3 A @ 4.8 V) plus the rest of the robot leaves little safety margin on a 1.5 A port. SG90 stall is typically higher still.

Leave the ESP32-C3-Zero **onboard USB-C unused** when the robot is assembled so two 5 V sources cannot fight on the same rail. That one cable supplies robot power and programming / serial CDC (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`, monitor 115200).

Advanced path (Adafruit 5993, CC resistors advertising up to ~1.5 A): [wiring.md](wiring.md#usb-connector-5993).

## Insufficient-power symptoms

If the rail sags under servo or audio load:

- ESP32 resets / brownouts
- Servo jitter or weak motion
- OLED glitches or blanking
- Audio noise / dropouts
- Unstable I2C (NACKs, PCA9685 or OLED “not found” after motion starts)
- Resets **specifically when multiple servos start moving**

Bring-up test turns all five servos together — that is a power-stress moment.

Related: [main-control-board.md](main-control-board.md), [components.md](components.md), [interfaces.md](interfaces.md). Advanced harness: [wiring.md](wiring.md).
