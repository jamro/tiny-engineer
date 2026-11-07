#!/usr/bin/env python3
"""Fallback JLCPCB-oriented package via kicad-cli (no Fabrication Toolkit).

Produces gerbers.zip, bom.csv, positions.csv under --out-dir.

Limitations vs Fabrication Toolkit:
- No AUTO TRANSLATE rotation database — verify orientation in JLCPCB preview.
- BOM LCSC column uses schematic fields "LCSC Part #" or "LCSC Part".

Requires kicad-cli on PATH (or --kicad-cli).
"""

from __future__ import annotations

import argparse
import csv
import io
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True)


def remap_positions(src: Path, dest: Path) -> None:
    text = src.read_text(encoding="utf-8-sig")
    reader = csv.DictReader(io.StringIO(text))
    if not reader.fieldnames:
        raise SystemExit(f"error: empty positions file {src}")
    # kicad-cli csv headers are typically Ref,Val,Package,PosX,PosY,Rot,Side
    rows_out = []
    for row in reader:
        ref = row.get("Ref") or row.get("Designator")
        x = row.get("PosX") or row.get("Mid X")
        y = row.get("PosY") or row.get("Mid Y")
        rot = row.get("Rot") or row.get("Rotation")
        side = (row.get("Side") or row.get("Layer") or "").strip().lower()
        if side in ("front", "top", "f.cu"):
            layer = "top"
        elif side in ("back", "bottom", "b.cu"):
            layer = "bottom"
        else:
            layer = side or "top"
        rows_out.append(
            {
                "Designator": ref,
                "Mid X": x,
                "Mid Y": y,
                "Rotation": rot,
                "Layer": layer,
            }
        )
    dest.parent.mkdir(parents=True, exist_ok=True)
    with dest.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(
            f,
            fieldnames=["Designator", "Mid X", "Mid Y", "Rotation", "Layer"],
        )
        w.writeheader()
        w.writerows(rows_out)


def export_bom(kicad_cli: str, sch: Path, dest: Path) -> None:
    # Prefer LCSC Part #; KiCad exports empty if field missing on some symbols.
    # Include both field names via comma — unknown fields become empty columns.
    fields = "Reference,Footprint,${QUANTITY},Value,LCSC Part #,LCSC Part"
    labels = "Designator,Footprint,Quantity,Value,LCSC Part #,LCSC Part Alt"
    tmp = dest.with_suffix(".raw.csv")
    run(
        [
            kicad_cli,
            "sch",
            "export",
            "bom",
            "--output",
            str(tmp),
            "--fields",
            fields,
            "--labels",
            labels,
            "--group-by",
            "Value,Footprint,LCSC Part #,LCSC Part",
            "--exclude-dnp",
            str(sch),
        ]
    )
    text = tmp.read_text(encoding="utf-8-sig")
    reader = csv.DictReader(io.StringIO(text))
    rows = []
    for row in reader:
        lcsc = (row.get("LCSC Part #") or row.get("LCSC Part Alt") or "").strip()
        rows.append(
            {
                "Designator": row.get("Designator", ""),
                "Footprint": row.get("Footprint", ""),
                "Quantity": row.get("Quantity", ""),
                "Value": row.get("Value", ""),
                "LCSC Part #": lcsc,
            }
        )
    with dest.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(
            f,
            fieldnames=[
                "Designator",
                "Footprint",
                "Quantity",
                "Value",
                "LCSC Part #",
            ],
        )
        w.writeheader()
        w.writerows(rows)
    tmp.unlink(missing_ok=True)


def export_gerbers(kicad_cli: str, pcb: Path, dest_zip: Path) -> None:
    # Prefer explicit layers (Protel extensions). Avoid --check-zones: it has
    # aborted kicad-cli on some hosts. Toolkit path is still preferred when available.
    with tempfile.TemporaryDirectory() as td:
        out = Path(td)
        run(
            [
                kicad_cli,
                "pcb",
                "export",
                "gerbers",
                "--output",
                str(out),
                "-l",
                "F.Cu,B.Cu,F.SilkS,B.SilkS,F.Mask,B.Mask,F.Paste,B.Paste,Edge.Cuts",
                str(pcb),
            ]
        )
        if not any(out.iterdir()):
            raise SystemExit("error: gerber export produced no files")
        run(
            [
                kicad_cli,
                "pcb",
                "export",
                "drill",
                "--output",
                str(out),
                "--format",
                "excellon",
                "--excellon-zeros-format",
                "decimal",
                "--excellon-units",
                "mm",
                "--generate-map",
                "--map-format",
                "gerberx2",
                str(pcb),
            ]
        )
        dest_zip.parent.mkdir(parents=True, exist_ok=True)
        if dest_zip.exists():
            dest_zip.unlink()
        with zipfile.ZipFile(dest_zip, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            for p in sorted(out.rglob("*")):
                if p.is_file():
                    zf.write(p, arcname=p.name)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--board-dir", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--kicad-cli", default="kicad-cli")
    args = parser.parse_args()

    board_dir = args.board_dir.resolve()
    pcb = board_dir / "main-control-board.kicad_pcb"
    sch = board_dir / "main-control-board.kicad_sch"
    if not pcb.is_file() or not sch.is_file():
        print(f"error: missing pcb/sch under {board_dir}", file=sys.stderr)
        return 1

    kicad_cli = args.kicad_cli
    if not shutil.which(kicad_cli) and not Path(kicad_cli).is_file():
        print(f"error: kicad-cli not found: {kicad_cli}", file=sys.stderr)
        return 1

    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as td:
        tdir = Path(td)
        pos_raw = tdir / "pos.csv"
        run(
            [
                kicad_cli,
                "pcb",
                "export",
                "pos",
                "--output",
                str(pos_raw),
                "--format",
                "csv",
                "--units",
                "mm",
                "--side",
                "both",
                "--exclude-dnp",
                str(pcb),
            ]
        )
        remap_positions(pos_raw, out / "positions.csv")

    export_bom(kicad_cli, sch, out / "bom.csv")
    export_gerbers(kicad_cli, pcb, out / "gerbers.zip")

    for name in ("gerbers.zip", "bom.csv", "positions.csv"):
        p = out / name
        if not p.is_file() or p.stat().st_size == 0:
            print(f"error: missing or empty {p}", file=sys.stderr)
            return 1
        print(f"ok: {p} ({p.stat().st_size} bytes)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
