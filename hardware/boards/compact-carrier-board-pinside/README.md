# compact-carrier-board-pinside

**The pin-side (mirrored) revision of
[`compact-carrier-board`](../compact-carrier-board/README.md), kept because it is
what the boards in transit physically are.** Do not order it unless you
specifically want the build described below.

- **Status:** frozen — archival variant, not developed further
- **Artwork:** byte-identical to `compact-carrier-board` as of commit `ddeb0d0`
  (md5 `26995498ccf56c28db661f4333d1f64c` — checksum of a public board file, secret-scan:allow), i.e. the
  `carrier-fab-v3-replacement_Y5` upload JLCPCB produced and shipped
- **KiCad:** 10

## What is different

Its socket pad map is the ESP32-C3-Zero seen from the **pin side**: `J_ESP1`
(5V, GND, 3V3, GPIO0-GPIO5) sits on the OUTER row nearest the slot edge and
`J_ESP2` (GPIO21-GPIO6, D+/D- on pins 16/15) on the inner row. The corrected
board has those two rows the other way round.

A pin-side map is the correct drawing only for a socket on the **opposite
face** from the one being drawn — which is why this board mates only when
built the following way.

## How to build it

1. Solder both Kinghelm KH-2.54FH-1X9P-H3.5 female strips to the **pour face**
   (B.Cu — the GND-fill side, the one with no silkscreen).
2. Run the board with that face **up**. From the pour face you see the mirror
   of the F.Cu artwork, which is the module's component-side map, so a
   component-side-up module mates correctly.
3. Mark pin 1 before assembly — the silkscreen ends up underneath. Pin 1 (5V)
   is the square pad, at the corner of the socket array that the wide 5V trace
   runs to, and that trace is visible on the pour face.
4. Plugged in 180° out, 5V lands on GPIO5.

## What that build costs

- The module's own USB-C overhangs the long edge facing the **opposite wall**
  from the corrected board's intent. Unused either way — the Adafruit 5993
  jack carries power and CDC, see
  [`docs/hardware/interfaces.md`](../../../docs/hardware/interfaces.md).
- Silkscreen faces down, so the reference designators are unreadable after
  assembly, and this artwork has no pin-name silk to lose in the first place.
- The wire-end connector positions mirror left-right, so the runs to the
  PCA9685 / OLED / amp swap sides relative to the corrected board.
- The corrected board's routing fixes are absent here, measured on this file:
  9mm of the 5V net sits at 0.600mm against its own ≈0.78mm IPC-2221 target
  (total length 83.901mm), and the worst track-to-pad clearance is 0.270mm.

## Choosing between the two

| | `compact-carrier-board` | this variant |
| --- | --- | --- |
| Headers soldered to | silk face (F.Cu) | pour face (B.Cu) |
| Face up in the desk | silk | pour |
| Module USB-C overhangs | the 5V-bus long edge | the opposite long edge |
| Pin names on silk | yes | no |
| 5V net | 44.939mm, all 1.000mm | 83.901mm, 9mm of it 0.600mm |
| Worst track-to-pad | 0.420mm | 0.270mm |
| Use it when | ordering new boards | you have the in-transit batch in hand |

`kicad-cli pcb drc --severity-all` on this file reports 0 violations and 0
unconnected items. It is electrically sound; it is the row assignment that is
inverted, and DRC cannot see that — only handedness can.

## License

Board sources in `boards/` are [CERN-OHL-S-2.0](../../LICENSE). See
[NOTICE](../../NOTICE) for copyright, Source Location, and product notice
requirements.
