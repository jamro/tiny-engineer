# Main control board

Builder connect guide for the integrated PCB path. Same firmware and pins as the modular harness. Decide path and cart: [shopping.md](../shopping.md#choose-electronics-path).

The board is **open source** ([CERN-OHL-S](../../3d_models/LICENSE)). KiCad sources: [`hardware/boards/main-control-board/`](../../hardware/boards/main-control-board/). Export Gerbers and order from any fab yourself. The [interest check](https://github.com/jamro/tiny-engineer/discussions/50) is optional — a community batch for easier / cheaper buys, not a gate to use the design.

On the board: PCA9685, MAX98357A, USB-C (power + data). Still plug in: ESP32-C3-Zero, OLED, five servos, speaker. Electrical detail for contributors: the board README in that folder.

Seat the PCB in the printed `Desk` during [assembly §12–13](../3d/assembly.md#12-electronics-inside-the-desk). This page is the electrical plug-in procedure.

## Connect

**Power / flash / serial** use the board **USB-C** (data cable). Do not also wire a separate Adafruit 5993.

1. **ESP32-C3-Zero** — Plug into the top headers. Module pins pass through the dedicated cut in the desk top (see assembly). Orient using the **USB** marking on the PCB silkscreen so the module matches the cut and USB side.
2. **OLED** — Plug into the male header labeled **OLED**.
3. **Servos** — Plug into the labeled servo sockets. Match socket orientation (signal / 5 V / GND as labeled). Which joint → which channel: [pinout.md](pinout.md#pca9685-channels-not-esp32-gpio).
4. **Speaker** — Use **one** method only, never both:
   - **Connector:** stock Adafruit Mini Oval — plug as labeled. A non-Adafruit / alternate speaker may work; check polarity. If phase is wrong or the plug pinout differs, swap wires in the connector.
   - **Solder pads:** solder the speaker leads to the dedicated speaker pads on the board.
   - **Never** connect both the connector and the solder pads at the same time.

## Pre-power checks

- Speaker: connector **or** pads, not both. **SPK− is not ground.**
- Servo plugs oriented to the labels; leave channels unplugged until centering if you prefer.
- ESP32 seated in the correct orientation (PCB USB mark).
- Supply **5 V / ≥2 A** on the board USB-C.

Same voltage rules as [getting-started Safety](../getting-started.md#safety). Power architecture: [power.md](power.md).

## Next

[Flash](../flash.md) on the desk, then continue [getting-started](../getting-started.md) → assemble.
