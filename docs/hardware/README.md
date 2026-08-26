# Tiny Engineer — hardware overview

Canonical hardware reference for the Tiny Engineer robot.

Firmware pin constants live in [`include/pins.h`](../../include/pins.h). Wiring topology lives in [`docs/wiring/Tiny Engineer.drawio.png`](../wiring/Tiny%20Engineer.drawio.png).

Conflict order: firmware GPIO numbers → wiring PNG connections → this `docs/hardware/` set.

| Audience | Use this set for |
| --- | --- |
| Human | Assembly, wiring, power, bring-up, debugging |
| Firmware agent | Pin map, buses, voltages, servo PWM conventions, test sequence |

## Purpose

Small desktop robot with:

- 5 analog micro servos (head, neck, hands, body — [robot-movement.md](../robot-movement.md))
- I2C OLED status display
- I2S speaker for tones and WAV playback
- ESP32-C3 Wi-Fi for HTTP control and AI hooks

Firmware (`src/main.cpp`) is the robot application: Wi-Fi, settings, hardware tests, animations, and the JSON API.

## Hardware architecture

One ESP32-C3 module owns all logic. Servo PWM is offloaded to a PCA9685 so pulse generation does not depend on ESP32 timing. Audio is I2S into a class-D amp. Display and servo driver share one I2C bus.

```mermaid
flowchart TB
  USB["USB 5V / GND<br/>as drawn"]

  subgraph p5["5 V power domain"]
    ESP5["ESP32 5V"]
    PCA5["PCA9685 5V"]
    PCAVplus["PCA9685 V+"]
    MAXVIN["MAX98357A Vin"]
    SERVOS["Servo 5V"]
  end

  subgraph p33["3.3 V logic domain"]
    ESP["ESP32-C3-Zero"]
    PCA["PCA9685 VCC"]
    OLED["OLED VCC"]
  end

  USB -->|"5V"| ESP5
  USB -->|"5V"| PCA5
  USB -->|"5V"| MAXVIN
  PCA5 --> PCAVplus
  PCAVplus --> SERVOS
  ESP5 --> ESP
  ESP -->|"3V3"| PCA
  ESP -->|"3V3"| OLED
  ESP -->|"GP0 SDA / GP1 SCL"| PCA
  ESP -->|"GP0 SDA / GP1 SCK"| OLED
  ESP -->|"GP2 BCLK / GP3 LRC / GP4 DIN"| MAXVIN
  PCA -->|"PWM"| SERVOS
```

Speaker **SPK+/SPK-** are not on the PNG. See [wiring.md](wiring.md#not-on-the-drawing).

| Domain | Voltage | What lives there |
| --- | --- | --- |
| Power rail | **+5V** | USB **5V** → ESP32 **5V**, PCA9685 **5V**, MAX98357A **Vin**; PCA9685 **V+** → servo **5V** |
| Logic | **3V3** | ESP32 GPIO, PCA9685 **VCC**, OLED **VCC**, I2C, I2S |
| Ground | **GND** | Every module — common ground is mandatory |

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator.

> [!WARNING]
> MAX98357A SPK- is not ground. Speaker connects between SPK+ and SPK- only.

## Major components

| Role | Part | Qty |
| --- | --- | --- |
| Controller | Waveshare ESP32-C3-Zero | 1 |
| Servo PWM | Adafruit PCA9685 16-channel driver | 1 |
| Actuators | PowerHD HD-1370A analog micro servo | 5 |
| Audio amp | MAX98357A I2S class-D (mono) | 1 |
| Speaker | 8 Ω / 1 W mono | 1 |
| Display | 0.91" 128×32 OLED, SSD1306, I2C | 1 |
| Robot USB | Adafruit 5993 USB-C breakout (power + data) | 1 |

Full inventory: [components.md](components.md).

## Communication buses

| Bus | ESP32 pins | Devices |
| --- | --- | --- |
| I2C | GP0/SDA, GP1/SCL (OLED pad **SCK**) | PCA9685 `0x40`, SSD1306 `0x3C` |
| I2S | GP2/BCLK, GP3/LRC, GP4/DIN | MAX98357A |
| Servo PWM | *(none on ESP32)* | PCA9685 channels 0–4 @ 50 Hz |
| USB | Adafruit 5993 (VBUS/GND + D+/D− → GP19/GP18) | Power, flash, serial CDC |

Details: [interfaces.md](interfaces.md), [pinout.md](pinout.md).

## Power architecture

Single nominal **+5V** supply via the Adafruit 5993. ESP32 onboard LDO makes **3V3** for logic only. PCA9685 **V+** (servos) is electrically separate from PCA9685 **VCC** (logic).

Adafruit 5993 CC resistors request **5 V / up to ~1.5 A**. Five stalled servos alone can approach that. Prefer a **5 V / ≥2 A** source with margin.

Details: [power.md](power.md).

## Documentation map

| File | Contents |
| --- | --- |
| [components.md](components.md) | Inventory, voltages, limits |
| [pinout.md](pinout.md) | GPIO map + allocation rules |
| [wiring.md](wiring.md) | Every electrical connection |
| [power.md](power.md) | Budget, brownout symptoms, single-USB policy |
| [servos.md](servos.md) | PWM, test limits, safe ranges |
| [interfaces.md](interfaces.md) | I2C / I2S / PWM / USB |
| [testing.md](testing.md) | Bring-up sequence, failures, what to check |

Existing schematic sketch (not a substitute for the tables here): [`docs/wiring/Tiny Engineer.drawio`](../wiring/Tiny%20Engineer.drawio) / [PNG](../wiring/Tiny%20Engineer.drawio.png).

## Source-of-truth order

1. Repository firmware / `include/pins.h`
2. This `docs/hardware/` set and `docs/wiring/`
3. Component datasheets
