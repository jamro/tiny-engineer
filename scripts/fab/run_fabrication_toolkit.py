#!/usr/bin/env python3
"""Run Fabrication Toolkit headlessly and wait for completion.

Primary path for JLCPCB-oriented Gerber ZIP + bom.csv + positions.csv.
Requires KiCad's Python with pcbnew (Toolkit also imports wx).

Spike (macOS KiCad 10.0.6 + Toolkit 5.3.x): ProcessThread.join() writes
production/{bom.csv,positions.csv,*.zip}.

FAB_TOOLKIT_ROOT must be the release root that contains plugins/
(or a shim with plugins/ -> PCM install). Prefer scripts/fab/export_*.sh
to prepare that root.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Headless Fabrication Toolkit export (JLCPCB Gerber/BOM/CPL)."
    )
    parser.add_argument("--board", required=True, type=Path, help="Path to .kicad_pcb")
    parser.add_argument(
        "--toolkit-root",
        type=Path,
        default=None,
        help="Toolkit release root containing plugins/ (or FAB_TOOLKIT_ROOT)",
    )
    args = parser.parse_args()

    board = args.board.resolve()
    if not board.is_file():
        print(f"error: board not found: {board}", file=sys.stderr)
        return 1

    toolkit_root = Path(
        args.toolkit_root or os.environ.get("FAB_TOOLKIT_ROOT") or ""
    )
    if not toolkit_root.is_dir():
        print(
            "error: pass --toolkit-root or set FAB_TOOLKIT_ROOT to a Toolkit "
            "release root containing plugins/",
            file=sys.stderr,
        )
        return 1
    plugins = toolkit_root.resolve() / "plugins"
    if not (plugins / "cli.py").is_file():
        print(f"error: {plugins}/cli.py not found", file=sys.stderr)
        return 1
    sys.path.insert(0, str(toolkit_root.resolve()))

    try:
        import pcbnew  # type: ignore
    except ImportError:
        print(
            "error: pcbnew not importable — use KiCad's bundled Python "
            f"(got {sys.executable})",
            file=sys.stderr,
        )
        return 1

    # Fabrication Toolkit imports wx at module load even for CLI. Lite AppImage
    # sometimes lacks wx — provide a minimal stub so ProcessThread can run.
    try:
        import wx  # type: ignore  # noqa: F401
    except ImportError:
        import types

        class _PyEvent:
            def __init__(self) -> None:
                self._type = 0

            def SetEventType(self, t: int) -> None:
                self._type = t

        class _WxStub(types.ModuleType):
            def __init__(self) -> None:
                super().__init__("wx")
                self.OK = 4
                self.ICON_ERROR = 512
                self.PyEvent = _PyEvent
                self._id = 1000

            def NewId(self) -> int:
                self._id += 1
                return self._id

            def MessageBox(self, *args: object, **kwargs: object) -> None:
                return None

            def PostEvent(self, *args: object, **kwargs: object) -> None:
                return None

        sys.modules["wx"] = _WxStub()
        print("warn: using wx stub (real wx not installed)", flush=True)

    from plugins.options import (  # type: ignore
        ALL_ACTIVE_LAYERS_OPT,
        ALTERNATIVE_EDGE_CUT_OPT,
        ARCHIVE_NAME,
        AUTO_FILL_OPT,
        AUTO_TRANSLATE_OPT,
        BACKUP_OPT,
        EXCLUDE_DNP_OPT,
        EXTEND_EDGE_CUT_OPT,
        EXTRA_LAYERS,
    )
    from plugins.thread import ProcessThread  # type: ignore

    # BACKUP_OPT True => skip Toolkit's timestamped backups (inverted naming).
    options = {
        AUTO_TRANSLATE_OPT: True,
        AUTO_FILL_OPT: True,
        EXCLUDE_DNP_OPT: True,
        EXTEND_EDGE_CUT_OPT: False,
        ALTERNATIVE_EDGE_CUT_OPT: False,
        ALL_ACTIVE_LAYERS_OPT: False,
        ARCHIVE_NAME: "",
        EXTRA_LAYERS: None,
        BACKUP_OPT: True,
    }

    print(f"pcbnew {pcbnew.Version()}", flush=True)
    print(f"board  {board}", flush=True)
    thread = ProcessThread(
        wx=None,
        cli=str(board),
        options=options,
        openBrowser=False,
        nonInteractive=True,
    )
    thread.join()
    if thread.is_alive():
        print("error: Fabrication Toolkit thread still alive", file=sys.stderr)
        return 1

    production = board.parent / "production"
    required = ("bom.csv", "positions.csv")
    missing = [n for n in required if not (production / n).is_file()]
    zips = []
    if production.is_dir():
        zips = [
            z
            for z in production.glob("*.zip")
            if "backup" not in z.as_posix().lower()
        ]
    if missing or not zips:
        print(
            f"error: expected {required} and a Gerber zip under {production}; "
            f"missing={missing} zips={zips}",
            file=sys.stderr,
        )
        return 1
    print(f"ok: {production}", flush=True)
    for p in sorted(production.iterdir()):
        if p.is_file():
            print(f"  {p.name} ({p.stat().st_size} bytes)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
