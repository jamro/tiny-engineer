---
name: easyeda-to-kicad
description: >-
  Imports LCSC/EasyEDA parts into a Tiny Engineer KiCad board via
  easyeda2kicad --full (symbol, footprint, 3D model). Use when adding
  JLCPCB/LCSC parts, EasyEDA libraries, or filling
  hardware/boards/<board>/libraries.
---

# EasyEDA → KiCad board

Import an LCSC part into `hardware/boards/<board_name>/`. Library files only — do **not** place the symbol on the schematic. Do **not** merge into `TinyEngineerModules`.

## Inputs

| Input | Meaning |
| --- | --- |
| Part number | LCSC id starting with `C` (e.g. `C2040`) |
| Board | `hardware/boards/<board_name>/` (directory name = KiCad project) |

Fail if the board dir has no `.kicad_pro`. Manufacturer PNs are not valid; find the LCSC `C…` id first.

## Run

Execute this script (do not reimplement the download/move/lib-table steps):

```bash
python3 .cursor/skills/easyeda-to-kicad/scripts/import_lcsc.py \
  --lcsc-id C2040 \
  --board main-control-board
```

`--board` accepts `main-control-board` or `hardware/boards/main-control-board`.

The script:

1. Resolves the board from the repo root; creates `libraries/{symbols,footprints,3d}`.
2. Installs `easyeda2kicad` if missing.
3. Runs from the **board directory** (required for `--project-relative`):

```bash
python3 -m easyeda2kicad --full --lcsc_id=Cxxxx \
  --output libraries/symbols/easyeda \
  --project-relative --overwrite
```

Always `--full`. Always `--overwrite`. Never `--use-cache`.

4. Moves `libraries/symbols/easyeda.pretty/` → `libraries/footprints/easyeda.pretty/` and `libraries/symbols/easyeda.3dshapes/` → `libraries/3d/easyeda.3dshapes/`.
5. Rewrites 3D paths in footprints to `${KIPRJMOD}/libraries/3d/easyeda.3dshapes`.
6. Upserts lib nickname `easyeda` in `sym-lib-table` / `fp-lib-table`.

## Layout

```
hardware/boards/<board>/
  libraries/symbols/easyeda.kicad_sym
  libraries/footprints/easyeda.pretty/
  libraries/3d/easyeda.3dshapes/
  sym-lib-table
  fp-lib-table
```

Lib nickname **must** be `easyeda` (symbol footprint field is `easyeda:…`). URIs use `${KIPRJMOD}` only — no absolute local paths.

## Example

Import C2040 into main-control-board:

```bash
python3 .cursor/skills/easyeda-to-kicad/scripts/import_lcsc.py \
  --lcsc-id C2040 \
  --board main-control-board
```
