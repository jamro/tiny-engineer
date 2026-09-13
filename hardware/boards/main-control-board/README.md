# main-control-board

Carrier PCB that reduces jumper wiring: existing modules plug in on pin headers.

- **Status:** draft
- **KiCad:** 10
- **Interfaces:** header sockets for the Waveshare ESP32-C3-Zero, Adafruit PCA9685, MAX98357A, SSD1306 OLED, Adafruit 5993 USB-C breakout, and five servo 3-pin plugs; shared I2C (ESP32 GP0/GP1 → PCA9685 and SSD1306 OLED); point-to-point I2S (ESP32 GP2/GP3/GP4 → MAX98357A BCLK/LRC/DIN); USB D+/D− (5993 → ESP32 GP19/GP18); MAX98357A SPK+/SPK− → speaker; +5V, 3V3, and GND
- **Assumptions:** nets follow [`docs/hardware/`](../../../docs/hardware/README.md) (keep servo **+5V** off logic **3V3**; common GND). Schematic and PCB are empty KiCad 10 stubs. Custom module symbols live in [`libraries/symbols/tiny_engineer_modules.kicad_sym`](libraries/symbols/tiny_engineer_modules.kicad_sym). ESP32-C3-Zero symbol pin numbers are Waveshare **header pads** (pad 1 = 5V, numbered anticlockwise from USB), not the ESP32-C3 QFN package.
- **ERC / DRC:** empty schematic and PCB; both checks clean (no nets, no copper). Re-run when schematic and layout land.
- **Built:** not manufactured
- **Tested:** not tested

## Direction

First revision is sockets only. Design in KiCad 10, then fabricate, so the robot is wired on a board instead of Dupont harnesses.

Later revisions may integrate some or all of those functions onto this PCB and drop the plug-in modules. Electrical reference stays in `docs/hardware/` until a schematic is merged.

Contribution rules: [`docs/pcb.md`](../../../docs/pcb.md).
