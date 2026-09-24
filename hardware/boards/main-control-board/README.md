# main-control-board

This board drives the robot's servos and speaker. Power and USB data come in on a USB-C connector. The Waveshare ESP32-C3-Zero is not soldered down; it plugs into two 9-pin sockets.

- **Status:** draft (schematic and routed layout)
- **KiCad:** 10
- **Built:** not manufactured
- **Tested:** not tested

## Soldered on the board

The servo driver, the speaker amplifier, and the USB-C connector are parts on this PCB. They replace the plug-in PCA9685 board, the MAX98357A breakout, and the Adafruit 5993 USB-C breakout.

- **PCA9685** drives up to 16 PWM channels. This robot uses five of them for the servos. The I2C address is `0x40`: the address pins and the external-clock pin are tied to ground, so the chip uses its own clock.
- **Servo header** takes five servo plugs. Each plug has ground, 5 V, and a PWM signal.
- **MAX98357A** is the mono class-D amplifier. The speaker connects to a 2-pin header. The speaker's minus terminal is not ground. Connect the speaker only between the two amplifier outputs.
- **USB-C connector** supplies 5 V and the USB data pair. Two 5.1 kΩ resistors on the CC pins mark this board as a USB device. The charger decides how much current is available.
- A **3 A resettable fuse** sits between USB power and the 5 V rail.
- **OLED header** is for the SSD1306 display. Pin order, from pin 1: 3.3 V, ground, SDA, SCL.
- A second header shares that I2C bus. Its pin order is different: ground, 3.3 V, SDA, SCL.
- A spare 4-pin header is on the board for later use.

A solder jumper next to the amplifier is marked as a 6 dB gain limit.

## What still plugs in

The **Waveshare ESP32-C3-Zero** plugs into two 9-pin sockets, one for each row of pins on the module. This board has no ESP32 chip of its own.

The OLED module plugs into the OLED header.

## Power

USB-C 5 V goes through the fuse onto the 5 V rail. That rail feeds the ESP32 5 V pin, the servo plugs, and a 5 V indicator LED.

The amplifier has its own 5 V net. A net tie joins it to the main 5 V rail, so the amplifier supply can be routed on its own.

3.3 V comes from the regulator on the ESP32 module. This board does not have a 3.3 V regulator. 3.3 V feeds the PCA9685 logic supply, the OLED, the I2C pull-ups (4.7 kΩ), the pull-up on the PCA9685 output-enable pin (10 kΩ), and a 3.3 V indicator LED.

Servo 5 V and logic 3.3 V stay separate. They share ground.

Use a **5 V supply of at least 2 A**. Five stalled servos can draw more than a typical USB port provides.

## Connections from the ESP32

| Function | ESP32 pin | Goes to |
| --- | --- | --- |
| I2C SDA | GP0 | PCA9685 and OLED |
| I2C SCL | GP1 | PCA9685 and OLED |
| I2S bit clock | GP2 | MAX98357A BCLK |
| I2S word clock | GP3 | MAX98357A LRC |
| I2S data | GP4 | MAX98357A DIN |
| Servo output enable | GP5 | PCA9685 output enable (active low) |
| USB D− | GPIO18 | USB-C |
| USB D+ | GPIO19 | USB-C |

The output-enable wire is on this board. Firmware still ignores it: `PCA9685_OE_WIRED` is false in [`include/pins.h`](../../../include/pins.h).

## Board

Two-layer FR4, about 1.6 mm thick.

5 V tracks are 1.5 mm wide. 3.3 V tracks are 0.4 mm. The amplifier 5 V net is 0.5 mm. USB data tracks use their own width.

The two ESP32 sockets follow the Waveshare header. Pad 1 is 5 V. Pads count anticlockwise from the module's USB connector. Those numbers are the module's header pads, not the pins of the ESP32-C3 chip.

## Library exceptions

### USB1 3D model (C2765186)

Footprint `USB-C-SMD_TYPE-C-16PIN-2MD-073` loads `USB-C-SMD_TYPE-C-6PIN-2MD-073.{wrl,step}`.

EasyEDA has no separate `…16PIN….wrl` for this LCSC part. Official package `c_para.3DModel` is `USB-C-SMD_TYPE-C-6PIN-2MD-073` (outline3D uuid `4ee8413127e64716b804db03d4b340ae`). Re-import via `import_lcsc.py --lcsc-id C2765186` reproduces the same mapping.

Do not rename or replace the model only to silence filename-mismatch reviews — this is intentional.

Contribution rules: [`docs/pcb.md`](../../../docs/pcb.md).
