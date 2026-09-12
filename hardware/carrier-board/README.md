# ESP32 carrier board (desk-mounted build)

A small distribution PCB that sits under the ESP32-C3-Zero where it mounts through
the desk's ESP slot, fanning its I2C/I2S/power pins out to the PCA9685, OLED,
MAX98357A amp, and servo power — instead of point-to-point wiring.

Sized against the desk generated with the **PowerHD HD-1370A** servo preset
(`servos.json`) — the SG90 and FS0307 presets produce different desk
dimensions per the parametric design docs, and this board's fit has **not**
been verified against either of them. Re-check the ESP slot dimensions in
your own exported desk model before relying on this outline if you're on a
different preset. See `docs/hardware/README.md` for the canonical net map
(5V/3.3V domains, GP0-GP4 assignments); this board
implements that map in copper.

## Files

- `carrier-fab-v2.zip` — Gerbers + Excellon drill, upload directly to a fab
  (JLCPCB, PCBWay, OSHPark, PCBWave, etc.)
- `carrier.kicad_pcb` — KiCad 10 board source
- `carrier-board-render.png` — top-copper preview

## Why this shape

Measured directly from an exported desk part (not estimated): the ESP mounting
slot is 25.0 × 18.5mm, and three of its four sides sit only 3.0–3.6mm from
full-height enclosure walls — too tight for any connector housing. Only the
front is open. That forced the layout:

- **26 × 44mm**, 2-layer.
- Back 18.5mm: 2×9 female socket for the ESP32-C3-Zero (2.54mm pitch,
  15.24mm row spacing — Waveshare's documented spec for the Zero board family,
  confirmed within 0.04mm of a direct caliper measurement).
- Front zone: `J_PWR`, `J_SERVO` (5V direct to PCA9685 V+, bypassing the ESP's
  own 3.3V regulator), `J_I2C_OLED`, `J_I2C_PCA` (separate connectors, bus
  shared via on-board copper — no external splitter cable), `J_I2S` (own
  on-board 5V feed for the amp).
- Everything routes fully on-board; nothing needs an off-board jumper.

## Pinout (verified against physical board silkscreen)

ESP32-C3-Zero, row nearest USB-C, top-to-bottom:
`5V, GND, 3V3, GPIO0(SDA), GPIO1(SCL), GPIO2(BCLK), GPIO3(LRCLK), GPIO4(DIN), GPIO5`

The other row (`GPIO21,20,19,18,10,9,8,7,6`) is unused by this design — its
socket pads are mechanical/GND support only.

## Fab notes

- 0 DRC errors, 0 unconnected nets (KiCad 10).
- Tightest feature: 0.15mm copper (GND pour minimum thickness). Comfortably
  inside JLCPCB's 0.127mm floor; sits exactly at PCBWave's 0.15mm floor (their
  recommended safe minimum is 0.20mm) — check their DFM report before ordering
  if using PCBWave.
- Connectors are plain 2.54mm through-hole pads (pin headers + jumpers), not
  JST — real JST-PH footprint dimensions weren't verified at design time.
- 5V trace sized per IPC-2221 (2A, 1oz copper, 10°C rise → ≈0.78mm minimum;
  routed at 1.0mm, 0.6mm only through one short pinch point).
