---
name: verify-jlcpcb-bom
description: >-
  Verifies a KiCad board production/bom.csv against schematic fields, PCB
  footprints, and the JLCPCB LCSC catalog (package, value, footprint). Use
  when checking a board BOM, LCSC part numbers, JLCPCB parts, or whether
  production/bom.csv matches the schematic and footprints.
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
