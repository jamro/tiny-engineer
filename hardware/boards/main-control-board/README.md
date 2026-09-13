# main-control-board

Carrier PCB that reducing jumper wiring: existing modules plug in on pin headers.

- **Status:** draft
- **KiCad:** 10
- **Interfaces:** header sockets for the Waveshare ESP32-C3-Zero, Adafruit PCA9685, MAX98357A, SSD1306 OLED, Adafruit 5993 USB-C breakout, and five servo 3-pin plugs; shared I2C (ESP32 GP0/GP1 → PCA9685 and SSD1306 OLED); point-to-point I2S (ESP32 GP2/GP3/GP4 → MAX98357A BCLK/LRC/DIN); USB D+/D− (5993 → ESP32 GP19/GP18); MAX98357A SPK+/SPK− → speaker; +5V, 3V3, and GND
- **Assumptions:** nets follow [`docs/hardware/`](../../../docs/hardware/README.md) (keep servo **+5V** off logic **3V3**; common GND). No schematic or layout yet. KiCad project name will match this directory.
- **Built:** not manufactured
- **Tested:** not tested

## Direction

First revision is sockets only. Design in KiCad 10, then fabricate, so the robot is wired on a board instead of Dupont harnesses.

Later revisions may integrate some or all of those functions onto this PCB and drop the plug-in modules. Electrical reference stays in `docs/hardware/` until a schematic is merged.

Contribution rules: [`docs/pcb.md`](../../../docs/pcb.md).
