# compact-carrier-board

Small distribution PCB that sits under the ESP32-C3-Zero where it mounts through
the desk's ESP slot, fanning its I2C/I2S/power pins out to the PCA9685, OLED,
MAX98357A amp, and servo power — instead of point-to-point wiring.

- **Status:** in review
- **KiCad:** 10
- **Interfaces:** ESP32-C3-Zero 2×9 socket, `J_PWR` (5V/GND + external USB
  D+/D-), `J_SERVO`, `J_I2C_OLED`, `J_I2C_PCA`, `J_I2S`
- **Assumptions:** sized against the desk generated with the **PowerHD
  HD-1370A** servo preset (`servos.json`) — the SG90 and FS0307 presets
  produce different desk dimensions per the parametric design docs, and this
  board's fit has **not** been verified against either of them. Re-check the
  ESP slot dimensions in your own exported desk model before relying on this
  outline if you're on a different preset. See `docs/hardware/interfaces.md`
  for the canonical net map (5V/3.3V domains, GP0-GP4 assignments); this
  board implements that map in copper.
- **Built:** the `carrier-fab-v3-replacement_Y5` upload was produced and
  shipped by JLCPCB (not yet received). That batch carries the **mirrored
  socket map this revision fixes** — see "Mirrored socket map (boards in
  transit)" below for the assembly workaround. Its drilling is fine: the Y5
  drill export is a single 0.9mm tool over all 37 holes and JLC's own CAM
  layer a single aperture, so the 7-undersized-hole defect of the earlier
  `_Y3` upload never reached a physical board.

## Files

- `compact-carrier-board.kicad_pcb` — KiCad 10 board source
- `compact-carrier-board.kicad_pro` — KiCad 10 project file (no `.kicad_sch`
  — this board has no schematic, laid out directly from footprints)
- `carrier-board-render.png` — 3D top render, regenerated with
  `kicad-cli pcb render --side top` after the row swap and silk additions

Gerbers, drill, BOM, and CPL are generated outputs, not source — regenerate
them from `compact-carrier-board.kicad_pcb` when ordering rather than
relying on a committed copy. `J_ESP1`/`J_ESP2` carry real `Value`/`LCSC`
fields and each footprint has a real origin at its own center, so
`kicad-cli pcb export pos` produces correct positions directly (verified:
`(13.00, -16.89)` / `(13.00, -1.65)`) — no manual patching. `J_PWR`,
`J_SERVO`, `J_I2C_OLED`, `J_I2C_PCA`, and `J_I2S` are flagged
`exclude_from_bom`/`exclude_from_pos_files` so they never appear in either
export. There's no `.kicad_sch`, so `kicad-cli` has no BOM-export
subcommand for this board (that's a schematic-only feature) — a BOM script
or fab-tool plugin reading the `.kicad_pcb`'s footprint properties directly
now gets correct `Value`/`LCSC` without hand-derivation.

## Assembly: only the ESP header strips are populated parts

`J_PWR`, `J_SERVO`, `J_I2C_OLED`, `J_I2C_PCA`, and `J_I2S` are bare
through-holes for direct wire connections, not connector footprints to
populate — excluded from BOM/CPL at the footprint level (see above).

`J_ESP1`/`J_ESP2` are the one real part: **Kinghelm KH-2.54FH-1X9P-H3.5**
(LCSC/JLCPCB `C55778388`), a 2.54mm 1×9 THT female header. There's no 2×9 part
— the
ESP32-C3-Zero's socket needs **two** of these strips, one per row. The board
models this as two separate footprint objects, **`J_ESP1`** (the inner row,
nearer the wire end: 5V/GND/3V3/GPIO0-5) and **`J_ESP2`** (the outer row,
nearest the slot edge: GPIO21-GPIO6), each a real
9-pad object with its own designator — not one 2×9 footprint with a
quantity hint in prose. JLCPCB's assembly tooling (and most BOM tooling)
determines quantity by counting designators, not by parsing Comment text, so
a generated BOM lists both on one row as `"J_ESP1,J_ESP2"` (identical part,
JLC's own convention for grouping identical components) — that reads as qty
2 unambiguously.

A generated CPL has one row per designator, `J_ESP1` and `J_ESP2`, each at
its own row's real center — (13.00, 16.89) and (13.00, 1.65) in the board's
own coordinates — read directly by `kicad-cli pcb export pos` from each
footprint's own origin, no hand-derivation needed.

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

## Pinout (rows stated in the frame that decides them)

Row placement is the part that is easy to get backwards, so name the rule:
**the pad map, seen from the face the module plugs into, has to be the
module's component-side view.** A pin-side view is the correct drawing only
for a socket on the *opposite* face — that inversion is exactly what went
wrong in the shipped batch (below).

Hold the C3-Zero component side up with its USB-C over this board's 5V-bus
edge (the long edge the 5V trace hugs). Then:

- **Outer row** — `J_ESP2`, nearest the slot edge (y=14.15):
  `GPIO21, GPIO20, GPIO19, GPIO18, GPIO10, GPIO9, GPIO8, GPIO7, GPIO6`.
  Mechanical/no-net support **except GP19 and GP18**, the native USB pair —
  see USB below.
- **Inner row** — `J_ESP1`, nearer the wire end (y=29.39):
  `5V, GND, 3V3, GPIO0(SDA), GPIO1(SCL), GPIO2(BCLK), GPIO3(LRCLK), GPIO4(DIN), GPIO5`

Pins 1 and 18 land at the 5V-bus edge in both rows, and that is what fixes
the module's USB-C to that edge. Traversing 1 to 18 reads counter-clockwise
from above, matching Waveshare's own pinout art (1-9 down the left column,
10-18 up the right, USB-C at the top).

## USB pass-through (external jack, not the C3-Zero's own USB-C)

Per `docs/hardware/interfaces.md`: the robot's single external USB-C (Adafruit
5993) carries both power and the ESP32's native USB D+/D- lines, so flashing
and serial CDC work through the panel-mount jack once the desk is sealed —
the C3-Zero's own onboard USB-C is left unused. No ESD or series-resistor
circuitry is specified for this link; it's a direct connection.

| `J_PWR` pin | Net | Goes to |
| --- | --- | --- |
| 1 | GND | common ground |
| 2 | D+ | ESP32 **GPIO19** (outer-row pad, native USB DP) |
| 3 | D− | ESP32 **GPIO18** (outer-row pad, native USB DM) |
| 4 | 5V | ESP32 5V rail, PCA9685 V+ (via `J_SERVO`), MAX98357A |

D+/D- run down the same 2.54mm grid columns as their own pads (x=20.42 and
22.96), which sits half a pitch off `J_I2C_OLED`/`J_I2C_PCA`'s grid, so they
never cross those connectors' pads on the way past. Since the row swap they
also pass the inner row, where the 3V3 and SDA holes occupy those two
columns, so each line takes one short lateral jog into the adjacent lane
(1.27mm off-axis, 0.42mm to the nearest pad edge) and returns — still F.Cu,
no vias. Each line measures 36.088mm and the two are exactly length-matched;
that is 16mm longer than before the swap, which is electrically unremarkable
at full-speed 12 Mbps.

## Mirrored socket map (boards in transit)

That artwork is preserved as its own board, [`compact-carrier-board-pinside`](../compact-carrier-board-pinside/README.md),
so the build the in-transit batch needs stays orderable and documented rather than
living only in this section's prose.

**The shipped boards (`carrier-fab-v3-replacement_Y5`) have the two socket
rows the wrong way round.** Their pad map is the C3-Zero seen from its *pin
side*, which is the correct drawing only for a socket on the opposite face;
as fabricated, a component-side-up module does not mate.

Caught by handedness, which no rotation can change: on the module
(component side, USB-C up) pins 1-9 run DOWN the left column and 10-18 UP
the right, so 1 to 18 is counter-clockwise; on those boards `J_ESP1` 1 to 9
runs left-to-right along the outer row and `J_ESP2` 10 to 18 right-to-left
along the inner row, i.e. clockwise.

**This revision fixes the source** — rows swapped in copper, nothing moved
mechanically. **For the boards in transit, fix it at assembly:** solder both
female strips to the **pour face** (B.Cu) and run that face up. From the
pour face you see the mirror of the F.Cu artwork, which is the
component-side map, so the module mates. What that build costs: the module's
own USB-C ends up over the long edge facing the opposite wall from this
revision's intent (unused either way — the Adafruit 5993 jack carries power
and CDC), the silkscreen ends up underneath so mark pin 1 before assembly,
and the wire-end connector positions mirror left-right, so the runs to the
PCA9685 / OLED / amp swap sides.

## Fab notes

- `kicad-cli pcb drc --severity-all` (KiCad 10.0.6, run *without*
  `--refill-zones` so it judges the on-disk fill) reports **0 violations and
  0 unconnected items** — and that statement now means something. The
  project's DRC floors used to be `min_clearance` 0.0, `min_silk_clearance`
  0.0, `min_text_height` 0.8 and `min_text_thickness` 0.08, so DRC could not
  have failed on clearance, silk overlap or undersized text however bad they
  got. They are now 0.2 / 0.15 / 1.0 / 0.15, matching JLCPCB's floors and
  its standard-font minimum. Schematic parity is not checked and cannot be —
  there is no `.kicad_sch` for this board, and `--schematic-parity` answers
  "Schematic parity tests require a fully annotated schematic".
- **Every pad on the board is named on silk.** The socket rows carry
  `GP21`-`GP6` (outer) and `5V, GND, 3V3, GP0`-`GP5` (inner) with a `USB`
  arrow pointing at the 5V-bus edge; each wire-end connector carries its own
  pin names (`3V3/SDA/SCL/GND` on both I2C headers, `5V/BCLK/LRC/DIN/GND` on
  `J_I2S`, `GND/D+/D-/5V` on `J_PWR`, `5V/GND` on `J_SERVO`). 48 silk items,
  all 1.0mm height / 0.15mm stroke. Placement is checked geometrically, not
  by DRC — 0 silk-to-silk overlaps, 0.421mm minimum silk-to-pad, 0.383mm
  minimum silk-to-edge. Before this the board carried nothing but seven
  reference designators, which is why a mirrored pad map was invisible both
  in the render and on the bare board.
- The seven reference designators were 0.8mm high with a 0.12mm stroke,
  under JLCPCB's standard-font minimum and passing DRC only because
  `min_text_thickness` was 0.08. All seven are now 1.0mm / 0.15mm.
  `J_I2C_PCA`, `J_PWR` and `J_SERVO` moved to clear the new pin-name bands,
  and every label is re-checked against the 0.30mm edge margin — three of
  them sit at exactly that margin, so the larger text boxes were nudged back
  inside rather than allowed to overhang.
- GND pour clearance raised 0.20 → 0.25mm — the zone's local clearance and
  the Default netclass together — and refilled. At 0.20 the fill sat at
  exactly JLCPCB's floor with nothing left for their etch tolerance; the
  nearest track-based constraint is 0.42mm, so the extra 0.05mm costs no
  geometry.
- 4 reference designators (`J_ESP1`, `J_ESP2`, `J_I2C_OLED`, `J_I2C_PCA`)
  were printing 0.2–0.6mm past the board's left edge — visually confirmed,
  not just a DRC technicality. Each footprint's own text bounding box (not
  an estimate) was used to shift the label right just enough to sit at a
  0.3mm margin from the edge; the render below reflects the fix.
- GND zone (B.Cu) outline is inset 0.5mm from the board edge, not 1mm as in
  earlier revisions — the 1mm inset put pads near the top edge (e.g. `J_ESP1`
  pin 2, GND) close enough to the zone's own boundary that a fresh zone
  refill could fail to route a thermal-relief connection to them, especially
  after the `J_ESP2` drill fix changed nearby pad geometry. Swept the inset
  from 1.0mm down to 0.3mm in 8 steps against the drill-fixed board, each
  with a fresh `kicad-cli pcb drc --refill-zones`: 1.0mm and 0.8mm both
  reproducibly fail with 1 unconnected pad; every value from 0.6mm down to
  0.3mm passes with 0. 0.5mm sits in the middle of that verified-safe range
  — well clear of the failure boundary and with real margin above JLCPCB's
  ~0.2–0.3mm copper-to-edge floor.
- Tightest feature: 0.15mm copper (GND pour minimum thickness). Comfortably
  inside JLCPCB's 0.127mm floor; sits exactly at PCBWave's 0.15mm floor (their
  recommended safe minimum is 0.20mm) — check their DFM report before ordering
  if using PCBWave.
- Connectors are plain 2.54mm through-hole pads (pin headers + jumpers), not
  JST — real JST-PH footprint dimensions weren't verified at design time.
- 5V trace sized per IPC-2221 (2A, 1oz copper, 10°C rise → ≈0.78mm minimum).
  The whole net is now 1.0mm over its 44.9mm length, with **no pinch points
  left**: the row swap moved the 5V pad off the outer row onto the inner one,
  15.24mm closer to its loads, which removed the squeeze past the mechanical
  pad row that previously forced two short 0.6mm sections.
  Any track change requires refilling the GND zone, and the refill must be
  done via KiCad itself (GUI Fill All Zones + save, or the
  `pcbnew.ZONE_FILLER` Python API) and re-verified with `kicad-cli pcb drc`
  run *without* `--refill-zones` — that flag only recomputes the fill
  in-memory for the check, it does not persist to the file, so a stale
  on-disk fill can pass DRC while still being wrong in the Gerbers. This bit
  us once already in this revision's history.
