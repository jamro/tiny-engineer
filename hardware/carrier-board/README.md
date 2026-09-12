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

- `carrier-fab-v3.zip` — Gerbers + Excellon drill, upload directly to a fab
  (JLCPCB, PCBWay, OSHPark, PCBWave, etc.)
- `carrier.kicad_pcb` — KiCad 10 board source
- `carrier-board-render.png` — top-copper preview
- `BOM.csv` / `CPL.csv` — assembly files, **J_ESP only** (see below)

## Assembly: only J_ESP is a populated part

`J_PWR`, `J_SERVO`, `J_I2C_OLED`, `J_I2C_PCA`, and `J_I2S` are bare
through-holes for direct wire connections, not connector footprints to
populate — `BOM.csv`/`CPL.csv` deliberately list nothing for them.

`J_ESP` is the one real part: **Kinghelm KH-2.54FH-1X9P-H3.5** (LCSC/JLCPCB
`C55778388`), a 2.54mm 1×9 THT female header. The board's `J_ESP` footprint
is a single 2×9 pad group, but physically needs **two** of these strips (one
per row) — there's no 2×9 part. `BOM.csv` notes the ×2 quantity against the
one `J_ESP` designator rather than implying a single 2×9 unit exists.

`CPL.csv`'s one `J_ESP` row is the combined 2×9 footprint's bounding-box
center (13.00, 9.27mm in the board's own coordinates), extracted directly
from `carrier.kicad_pcb`'s pad positions — `kicad-cli pcb export pos` reports
(0,0) for every footprint here (this board has no `.kicad_sch`, and every
footprint was written with its own placement at `(at 0 0)` with pads given
in absolute coordinates instead), so positions were pulled straight from the
pad data and Y was negated to match kicad-cli's own X/Y sign convention
(confirmed empirically: patched a known coordinate into a scratch copy and
diffed kicad-cli's export against it, rather than assuming the sign). For
hand-placing the two physical strips: row A centers at (13.00, 1.65), row B
at (13.00, 16.89) — both real values (`row_y_a`/`row_y_b` in the generator),
not estimates; the single CPL row just can't split into two without
inventing a second designator that doesn't exist in the board file.

## Why this shape

Measured directly from an exported desk part (not estimated): the ESP mounting
slot is 25.0 × 18.5mm, and three of its four sides sit only 3.0–3.6mm from
full-height enclosure walls — too tight for any connector housing. Only the
front is open. That forced the layout:

- **26 × 44mm**, 2-layer.
- Back 18.5mm: 2×9 female socket for the ESP32-C3-Zero (2.54mm pitch,
  15.24mm row spacing — Waveshare's documented spec for the Zero board family,
  confirmed within 0.04mm of a direct caliper measurement).
- Front zone: `J_PWR` (now 4-pin — 5V/GND plus the external USB D+/D-, see
  below), `J_SERVO` (5V direct to PCA9685 V+, bypassing the ESP's own 3.3V
  regulator), `J_I2C_OLED`, `J_I2C_PCA` (separate connectors, bus shared via
  on-board copper — no external splitter cable), `J_I2S` (own on-board 5V
  feed for the amp).
- Everything routes fully on-board; nothing needs an off-board jumper.

## Pinout (verified against physical board silkscreen)

ESP32-C3-Zero, row nearest USB-C, top-to-bottom:
`5V, GND, 3V3, GPIO0(SDA), GPIO1(SCL), GPIO2(BCLK), GPIO3(LRCLK), GPIO4(DIN), GPIO5`

The other row (`GPIO21,20,19,18,10,9,8,7,6`) is mechanical/GND support only
**except GP19 and GP18**, which are real signal pads — see USB below.

## USB pass-through (external jack, not the C3-Zero's own USB-C)

Per `docs/hardware/interfaces.md`: the robot's single external USB-C (Adafruit
5993) carries both power and the ESP32's native USB D+/D- lines, so flashing
and serial CDC work through the panel-mount jack once the desk is sealed —
the C3-Zero's own onboard USB-C is left unused. No ESD or series-resistor
circuitry is specified for this link; it's a direct connection.

| `J_PWR` pin | Net | Goes to |
| --- | --- | --- |
| 1 | GND | common ground |
| 2 | D+ | ESP32 **GPIO19** (row B pad, native USB DP) |
| 3 | D− | ESP32 **GPIO18** (row B pad, native USB DM) |
| 4 | 5V | ESP32 5V rail, PCA9685 V+ (via `J_SERVO`), MAX98357A |

D+/D- route dead straight from their row-B pads down to `J_PWR` — no jogging,
since `J_PWR`'s pin spacing was deliberately placed on the same 2.54mm grid
as the ESP footprint itself (offset from `J_I2C_OLED`/`J_I2C_PCA`'s grid by
half a pitch), so the two lines never cross OLED/PCA's pads on the way past.

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
