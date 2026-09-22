#!/usr/bin/env python3
"""Import an LCSC/EasyEDA part into a Tiny Engineer KiCad board project."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

LIB_NICKNAME = "easyeda"
OUTPUT_PREFIX = Path("libraries") / "symbols" / "easyeda"
SRC_PRETTY = Path("libraries") / "symbols" / "easyeda.pretty"
SRC_3D = Path("libraries") / "symbols" / "easyeda.3dshapes"
DST_PRETTY = Path("libraries") / "footprints" / "easyeda.pretty"
DST_3D = Path("libraries") / "3d" / "easyeda.3dshapes"
OLD_3D_PATH = "${KIPRJMOD}/libraries/symbols/easyeda.3dshapes"
NEW_3D_PATH = "${KIPRJMOD}/libraries/3d/easyeda.3dshapes"
SYM_URI = "${KIPRJMOD}/libraries/symbols/easyeda.kicad_sym"
FP_URI = "${KIPRJMOD}/libraries/footprints/easyeda.pretty"
LIB_DESCR = "LCSC/EasyEDA imports"


def die(message: str, code: int = 1) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(code)


def _is_repo_root(candidate: Path) -> bool:
    return (candidate / "hardware" / "boards").is_dir() and (
        (candidate / ".git").exists() or (candidate / "docs" / "pcb.md").is_file()
    )


def find_repo_root(start: Path) -> Path:
    searched = []
    for origin in (start, Path(__file__).resolve().parent):
        for candidate in (origin, *origin.parents):
            if candidate in searched:
                continue
            searched.append(candidate)
            if _is_repo_root(candidate):
                return candidate
    die(f"could not find repo root from {start}")


def resolve_board_dir(repo_root: Path, board: str) -> Path:
    board_name = Path(board.strip().rstrip("/")).name
    board_dir = repo_root / "hardware" / "boards" / board_name
    if not board_dir.is_dir():
        die(f"board directory not found: {board_dir}")
    pro_files = list(board_dir.glob("*.kicad_pro"))
    if not pro_files:
        die(f"no .kicad_pro in {board_dir}")
    return board_dir


def ensure_dirs(board_dir: Path) -> None:
    for rel in (
        Path("libraries") / "symbols",
        Path("libraries") / "footprints",
        Path("libraries") / "3d",
    ):
        (board_dir / rel).mkdir(parents=True, exist_ok=True)


def easyeda2kicad_cmd() -> list[str]:
    probe = [sys.executable, "-m", "easyeda2kicad", "--help"]
    result = subprocess.run(probe, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if result.returncode == 0:
        return [sys.executable, "-m", "easyeda2kicad"]
    if shutil.which("easyeda2kicad"):
        return ["easyeda2kicad"]
    return []


def ensure_easyeda2kicad() -> list[str]:
    cmd = easyeda2kicad_cmd()
    if cmd:
        return cmd
    print("easyeda2kicad not found; installing…")
    install = subprocess.run(
        [sys.executable, "-m", "pip", "install", "easyeda2kicad"],
        check=False,
    )
    if install.returncode != 0:
        die("failed to install easyeda2kicad")
    cmd = easyeda2kicad_cmd()
    if not cmd:
        die("easyeda2kicad installed but not runnable")
    return cmd


def run_easyeda2kicad(board_dir: Path, lcsc_id: str, cmd: list[str]) -> None:
    # Absolute --output: easyeda2kicad --project-relative does
    # Path(f"{output}.3dshapes").relative_to(cwd) and that fails on a
    # relative output path.
    output = (board_dir / OUTPUT_PREFIX).resolve()
    argv = [
        *cmd,
        "--full",
        f"--lcsc_id={lcsc_id}",
        f"--output={output}",
        "--project-relative",
        "--overwrite",
    ]
    print("running:", " ".join(argv))
    result = subprocess.run(argv, cwd=board_dir, check=False)
    if result.returncode != 0:
        die(f"easyeda2kicad failed with exit {result.returncode}")


def move_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.mkdir(parents=True, exist_ok=True)
    for item in src.iterdir():
        target = dst / item.name
        if target.exists():
            if target.is_dir():
                shutil.rmtree(target)
            else:
                target.unlink()
        shutil.move(str(item), str(target))
    if src.exists() and not any(src.iterdir()):
        src.rmdir()


def rewrite_3d_paths(pretty_dir: Path) -> None:
    if not pretty_dir.is_dir():
        return
    for mod in pretty_dir.glob("*.kicad_mod"):
        text = mod.read_text(encoding="utf-8")
        if OLD_3D_PATH not in text:
            continue
        mod.write_text(text.replace(OLD_3D_PATH, NEW_3D_PATH), encoding="utf-8")


def lib_entry(name: str, uri: str) -> str:
    return (
        f'\t(lib (name "{name}") (type "KiCad") (uri "{uri}") '
        f'(options "") (descr "{LIB_DESCR}"))'
    )


def upsert_lib_table(path: Path, table_tag: str, uri: str) -> None:
    entry = lib_entry(LIB_NICKNAME, uri)
    if not path.is_file():
        path.write_text(
            f"({table_tag}\n\t(version 7)\n{entry}\n)\n",
            encoding="utf-8",
        )
        return
    text = path.read_text(encoding="utf-8")
    if f'(name "{LIB_NICKNAME}")' in text:
        return
    if text.rstrip().endswith(")"):
        stripped = text.rstrip()
        # Insert before the final closing paren of the table.
        without_close = stripped[: stripped.rfind(")")]
        path.write_text(f"{without_close.rstrip()}\n{entry}\n)\n", encoding="utf-8")
        return
    die(f"could not parse lib table {path}")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Import an LCSC/EasyEDA part into a Tiny Engineer KiCad board."
    )
    parser.add_argument("--lcsc-id", required=True, help="LCSC id, e.g. C2040")
    parser.add_argument(
        "--board",
        required=True,
        help="Board name or hardware/boards/<board_name>",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv if argv is not None else sys.argv[1:])
    lcsc_id = args.lcsc_id.strip()
    if not lcsc_id.startswith("C"):
        die(f"lcsc id must start with C, got {lcsc_id!r}")

    repo_root = find_repo_root(Path.cwd().resolve())
    board_dir = resolve_board_dir(repo_root, args.board)
    ensure_dirs(board_dir)
    cmd = ensure_easyeda2kicad()
    run_easyeda2kicad(board_dir, lcsc_id, cmd)

    move_tree(board_dir / SRC_PRETTY, board_dir / DST_PRETTY)
    move_tree(board_dir / SRC_3D, board_dir / DST_3D)
    rewrite_3d_paths(board_dir / DST_PRETTY)

    upsert_lib_table(board_dir / "sym-lib-table", "sym_lib_table", SYM_URI)
    upsert_lib_table(board_dir / "fp-lib-table", "fp_lib_table", FP_URI)

    print(f"imported {lcsc_id} into {board_dir.relative_to(repo_root)}")
    print(f"  symbol:    {SYM_URI}")
    print(f"  footprint: {FP_URI}")
    print(f"  3d:        {NEW_3D_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
