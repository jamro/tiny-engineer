---
name: verify-jlcpcb-bom
description: >-
  Verifies a KiCad board production/bom.csv against schematic fields, PCB
  footprints, and the JLCPCB LCSC catalog, then interprets each FAIL and WARN,
  marks string-match false positives, and recommends a fix only for real
  mismatches. Use when checking a board BOM, LCSC part numbers, JLCPCB parts,
  or whether production/bom.csv matches the schematic and footprints.
---

# Verify JLCPCB BOM

Report only. Do not edit the schematic, PCB, BOM, or footprints.

`hardware/boards/<board>/production/` is gitignored. Read `bom.csv` from disk. Do not infer nets from schematic geometry.

## Run

Execute this script (do not reimplement parsing or the JLCPCB request):

```bash
python3 .cursor/skills/verify-jlcpcb-bom/scripts/verify_bom.py \
  --board main-control-board
```

`--board` accepts `main-control-board` or `hardware/boards/main-control-board`.

If the user did not name a board, run it for the only `hardware/boards/*/production/bom.csv`. If several exist, ask which board.

Missing `production/bom.csv`: tell the user to generate it with KiCad's Fabrication Toolkit. Do not invent a BOM.

## How to read the report

The script prints `FAIL` lines, then `WARN` lines, then one `PASS  N` line. `N` is BOM rows with no fail. Exit code 1 means at least one fail.

- **Fail** — wrong or missing LCSC id, BOM row disagrees with the schematic (designator, value, LCSC, footprint), schematic footprint was not pushed to the board, JLCPCB package is not in the footprint name, or a passive value disagrees with the catalog Resistance / Capacitance / Inductance.
- **Warn** — DNP part still in the BOM, JLCPCB stock is 0, part is extended (`expand`), schematic value is a label that does not contain the manufacturer part number, or a project-library footprint pad count disagrees with a catalog pin count.
- A lookup failure is a fail. It means package and value were not checked. Do not fill those in from memory.

Quote the script output. Do not dump the JSON response. Do not re-fetch each part unless the script could not reach JLCPCB.

After the quote, interpret every `FAIL` and `WARN`. Group lines that share one cause. Do not stop at the raw script text.

## Interpret

Verdict for each line: **real**, **false positive**, or **info**.

A lookup failure is neither a design bug nor a false positive. For refs whose lookup failed, say the catalog was not reached and skip package and value claims. Continue interpreting findings for all other refs.

### Package line is a name search

`package … not in footprint …` means JLCPCB `componentSpecificationEn` was not found as a substring of the footprint name. The script lowercases, then strips spaces, `_`, `-`, and `=`. It skips only these generic tokens: `smd`, `tht`, `plugin`, `dip`, `radial`, `axial`, `chip`, `connector`, `throughhole`, `through-hole`. It splits on ASCII `,` `;` `/` only. A fullwidth comma `，` glues the surrounding words into one token, so `Surface Mount，Right Angle` never matches a KiCad name.

`Surface Mount` is not in the skip list. It fails against footprints that say nothing, or only `SMD`.

Pitch `P=1mm` does not match the text `P1.00mm` (`p1mm` vs `p1.00mm`). Body `3x3` does not match `L3.0-W3.0`. Those are format misses, not evidence the land pattern differs.

### False positive

Call the package line a **false positive** when every stated dimension agrees, even if the words differ:

| JLCPCB wording | Same footprint wording |
| --- | --- |
| `1mm`, `P=1mm` | `P1.00mm`, `P1.0mm` |
| `3x3`, `EP(3x3)` | `L3.0-W3.0`, `3.0x3.0` (the `(3x3)` is the body, not the exposed-pad size) |
| `TQFN-16-EP` | `TQFN-16` plus `EP` in the name |
| `Right Angle`, `Horizontal` | the other of those two, on a connector |
| `Surface Mount` | `SMD`, or an SMD land pattern with no mount word |
| `D10xL10.5` | `10x10.5` |

Also a false positive when the footprint name contains the manufacturer part number (or the standard equivalent, such as `53261-0271` for a PicoBlade-style 1.25 mm header) and pitch, pin count, and orientation agree.

`could not compare value` and `package … not comparable` are inconclusive. Say what was not checked. Do not call them false positives.

### Real package mismatch

Keep **real** when any of these disagree:

- Pitch (`P1.25` vs `P2.00`, `P0.50` vs `P0.65`)
- Pin count or row shape (`1x02` vs `1x04`, TQFN-16 vs TQFN-20)
- Body size that is not a zero/format difference (`0603` vs `0805`, `3x3` vs `4x4`)
- Orientation (`Horizontal` / `Right Angle` vs `Vertical` / top-entry / straight)
- Mount (`SMD` vs `THT` / pin-header vertical)
- Connector family whose land pattern differs (PicoBlade vs PH, JST SH vs GH)

If the line is still ambiguous, read that ref's schematic footprint, value, and LCSC fields, and the footprint name on the board. Do not infer nets from coordinates. If it stays ambiguous, verdict **uncertain** and name the dimension that is missing. Do not downgrade that to a false positive.

### Other lines

| Line | Verdict | Meaning |
| --- | --- | --- |
| Passive `value … does not match JLCPCB Resistance/Capacitance/Inductance` | real | Schematic value and catalog value are different parts |
| Missing LCSC, LCSC not an id, LCSC not in catalog | real | BOM cannot be ordered as drawn |
| BOM designator, value, LCSC, or footprint disagrees with the schematic | real | Export is stale or the schematic field changed |
| Schematic footprint not on the board | real | PCB was not updated from the schematic |
| `stockCount is 0` | info | Purchasing risk. The part may still be the right design choice |
| `extended part (expand)` | info | JLCPCB extended library: higher price, longer lead. Not a schematic error |
| `schematic value "…" vs MPN …` | info | Value is a function label (`LED`, `Conn_OLED`, `3A PPTC`). Expected when the LCSC is intentional |
| `N pads, JLCPCB pins M` | real unless the extra pads are an exposed pad, mounting holes, or shield tabs | Open that footprint and count electrical pads before recommending a footprint swap |

## Recommend a fix

Recommend a fix only for **real** and for **info** the user would act on (stock 0, extended cost). False positives: `no change`.

Name the field and the direction. Do not invent a replacement LCSC id. If a different part is required, say to search JLCPCB for the same footprint, pitch, pin count, and value.

- Wrong passive value: set the schematic value to the catalog value, or replace the LCSC with the part whose Resistance / Capacitance / Inductance matches the schematic. Regenerate `production/bom.csv`.
- Wrong or missing LCSC: set `LCSC Part #` on the schematic symbol to the intended in-stock id, then regenerate the BOM.
- Footprint text or pad count disagrees with the ordered package: assign the footprint that matches that package, update the PCB from the schematic, regenerate the BOM.
- Stale BOM vs schematic: regenerate with Fabrication Toolkit. Do not hand-edit `bom.csv`.
- Extended or zero stock, and the land pattern is already right: optional search for a basic-library equivalent. No schematic edit until that part is chosen.
- Function label vs MPN: no rename.

## Reply

Use this shape. Quote the script output unchanged. One block per cause, not per ref, when the cause is the same.

```markdown
<script output>

## Reading
- **J1, J3 — false positive.** JLCPCB says right-angle SMD at 1.25 mm / 1.00 mm. Footprints say `Horizontal` and `P1.25mm` / `P1.00mm`. `Surface Mount，Right Angle` is one token because of the fullwidth comma, so the name search fails. Pitch and orientation agree. No change.
- **U3 — false positive.** `TQFN-16-EP(3x3)` is the same body as `TQFN-16_L3.0-W3.0-…-EP`. No change.
- **R1 — real.** Schematic 10k, catalog resistance 4.7k. Set the schematic value to 4.7k or pick the LCSC whose Resistance is 10k. Do not invent the id. Regenerate the BOM.
- **J1, U1 — info.** Extended library. No schematic change. Optional: search a basic-library part with the same footprint.

## Do next
Only real design or BOM changes. Omit this section when every line is a false positive or a no-change info.
```
