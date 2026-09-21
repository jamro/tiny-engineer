#!/usr/bin/env python3
"""Run KiCad ERC, DRC, and expected-nets checks for hardware/boards/*.

Requires KiCad 10 (kicad-cli). Stdlib only — no pip packages. Same entrypoint
locally and in CI:

    python3 scripts/check_pcb.py
    python3 scripts/check_pcb.py main-control-board
    python3 scripts/check_pcb.py --report-dir artifacts/pcb
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

KICAD_CLI_HINT = (
    "Install KiCad 10 and ensure kicad-cli is on PATH, "
    "or set KICAD_CLI to the binary. "
    "macOS: /Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
)


def find_repo_root(explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit.resolve()
    return Path(__file__).resolve().parent.parent


def find_kicad_cli() -> Path:
    env = os.environ.get("KICAD_CLI")
    if env:
        path = Path(env)
        if path.is_file():
            return path
        raise SystemExit(f"error: KICAD_CLI is set but not a file: {env}")

    which = shutil.which("kicad-cli") or shutil.which("kicad-cli.exe")
    if which:
        return Path(which)

    mac = Path("/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli")
    if mac.is_file():
        return mac

    for base in (
        Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "KiCad",
        Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "KiCad",
    ):
        if not base.is_dir():
            continue
        matches = sorted(base.glob("*/bin/kicad-cli.exe"), reverse=True)
        if matches:
            return matches[0]

    raise SystemExit(f"error: kicad-cli not found. {KICAD_CLI_HINT}")


def discover_boards(repo_root: Path, names: list[str] | None) -> list[Path]:
    boards_root = repo_root / "hardware" / "boards"
    if not boards_root.is_dir():
        raise SystemExit(f"error: missing boards directory: {boards_root}")

    if names:
        dirs = [boards_root / name for name in names]
    else:
        dirs = sorted(p for p in boards_root.iterdir() if p.is_dir())

    boards: list[Path] = []
    for board_dir in dirs:
        name = board_dir.name
        sch = board_dir / f"{name}.kicad_sch"
        pcb = board_dir / f"{name}.kicad_pcb"
        if not board_dir.is_dir():
            raise SystemExit(f"error: board directory not found: {board_dir}")
        if not sch.is_file() or not pcb.is_file():
            if names:
                raise SystemExit(
                    f"error: {name} missing {sch.name} and/or {pcb.name}"
                )
            continue
        boards.append(board_dir)

    if not boards:
        raise SystemExit("error: no KiCad boards found under hardware/boards/")
    return boards


# --- minimal s-expression parser for KiCad netlists ---


def _tokenize(text: str) -> list[str]:
    tokens: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c.isspace():
            i += 1
            continue
        if c in "()":
            tokens.append(c)
            i += 1
            continue
        if c == '"':
            i += 1
            buf: list[str] = []
            while i < n:
                if text[i] == "\\" and i + 1 < n:
                    buf.append(text[i + 1])
                    i += 2
                    continue
                if text[i] == '"':
                    i += 1
                    break
                buf.append(text[i])
                i += 1
            tokens.append("".join(buf))
            continue
        j = i
        while j < n and not text[j].isspace() and text[j] not in "()":
            j += 1
        tokens.append(text[i:j])
        i = j
    return tokens


def _parse_tokens(tokens: list[str]) -> Any:
    if not tokens:
        raise ValueError("empty sexpr")
    tok = tokens.pop(0)
    if tok == "(":
        out: list[Any] = []
        while tokens and tokens[0] != ")":
            out.append(_parse_tokens(tokens))
        if not tokens:
            raise ValueError("unclosed sexpr")
        tokens.pop(0)
        return out
    if tok == ")":
        raise ValueError("unexpected )")
    return tok


def parse_sexpr(text: str) -> Any:
    tokens = _tokenize(text)
    tree = _parse_tokens(tokens)
    if tokens:
        raise ValueError("trailing tokens in sexpr")
    return tree


def _children(node: list[Any], tag: str) -> list[list[Any]]:
    return [c for c in node[1:] if isinstance(c, list) and c and c[0] == tag]


def _child(node: list[Any], tag: str) -> list[Any] | None:
    found = _children(node, tag)
    return found[0] if found else None


def _atom(node: list[Any], tag: str) -> str | None:
    c = _child(node, tag)
    if c is None or len(c) < 2:
        return None
    return str(c[1])


def parse_netlist_pins(netlist_text: str) -> dict[str, set[str]]:
    """Return net name -> set of RefDes.PinNum:PinName tokens."""
    root = parse_sexpr(netlist_text)
    if not isinstance(root, list) or root[0] != "export":
        raise ValueError("netlist root is not (export ...)")

    pin_names: dict[tuple[str, str, str], str] = {}
    libparts = _child(root, "libparts")
    if libparts:
        for libpart in _children(libparts, "libpart"):
            pins = _child(libpart, "pins")
            if not pins:
                continue
            lib = _atom(libpart, "lib") or ""
            part = _atom(libpart, "part") or ""
            for pin in _children(pins, "pin"):
                num = _atom(pin, "num")
                name = _atom(pin, "name")
                if num and name:
                    pin_names[(lib, part, num)] = name

    ref_libpart: dict[str, tuple[str, str]] = {}
    components = _child(root, "components")
    if components:
        for comp in _children(components, "comp"):
            ref = _atom(comp, "ref")
            if not ref:
                continue
            libsource = _child(comp, "libsource")
            if libsource:
                lib = _atom(libsource, "lib") or ""
                part = _atom(libsource, "part") or ""
                ref_libpart[ref] = (lib, part)

    nets_out: dict[str, set[str]] = {}
    nets = _child(root, "nets")
    if not nets:
        return nets_out

    for net in _children(nets, "net"):
        name = _atom(net, "name")
        if not name:
            continue
        pins: set[str] = set()
        for node in _children(net, "node"):
            ref = _atom(node, "ref")
            pin_num = _atom(node, "pin")
            if not ref or not pin_num:
                continue
            lib_part = ref_libpart.get(ref)
            pin_name = None
            if lib_part:
                pin_name = pin_names.get((lib_part[0], lib_part[1], pin_num))
            if not pin_name:
                pf = _atom(node, "pinfunction")
                if pf and "_" in pf:
                    left, right = pf.rsplit("_", 1)
                    if right == pin_num:
                        pin_name = left
                if not pin_name:
                    pin_name = pin_num
            pins.add(f"{ref}.{pin_num}:{pin_name}")
        nets_out[name] = pins
    return nets_out


def load_expected_nets(path: Path) -> dict[str, list[str]]:
    """Parse constrained expected-nets.yml (stdlib only).

    Accepts: # comments, blank lines, top-level `nets:`, bare or double-quoted
    net keys, and `- Token` list items. Rejects anything else.
    Pin tokens: RefDes.PinNum:PinName (e.g. ESP1.1:5V).
    """
    text = path.read_text(encoding="utf-8")
    nets: dict[str, list[str]] = {}
    current: str | None = None
    seen_nets = False
    key_re = re.compile(r'^("(?:\\.|[^"\\])*"|[A-Za-z0-9_./+-]+)\s*:\s*$')
    item_re = re.compile(r"^-\s+(\S+)\s*$")

    for lineno, raw in enumerate(text.splitlines(), start=1):
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        stripped = line.strip()
        indent = len(line) - len(line.lstrip(" "))

        if indent == 0 and stripped == "nets:":
            if seen_nets:
                raise ValueError(f"{path}:{lineno}: duplicate 'nets:'")
            seen_nets = True
            current = None
            continue

        if not seen_nets:
            raise ValueError(
                f"{path}:{lineno}: expected top-level 'nets:' before other content"
            )

        if indent == 2:
            m = key_re.match(stripped)
            if not m:
                raise ValueError(
                    f"{path}:{lineno}: expected net key like GND: or \"+5V\":"
                )
            key = m.group(1)
            if key.startswith('"') and key.endswith('"'):
                key = key[1:-1].replace(r"\"", '"').replace(r"\\", "\\")
            if key in nets:
                raise ValueError(f"{path}:{lineno}: duplicate net {key!r}")
            nets[key] = []
            current = key
            continue

        if indent == 4 and current is not None:
            m = item_re.match(stripped)
            if not m:
                raise ValueError(
                    f"{path}:{lineno}: expected list item "
                    f"'- RefDes.PinNum:PinName'"
                )
            nets[current].append(m.group(1))
            continue

        raise ValueError(f"{path}:{lineno}: unsupported line: {raw!r}")

    if not seen_nets:
        raise ValueError(f"{path}: missing top-level 'nets:'")
    if not nets:
        raise ValueError(f"{path}: 'nets:' has no entries")
    return nets


def check_expected_nets(expected_path: Path, netlist_path: Path) -> list[str]:
    errors: list[str] = []
    try:
        expected_nets = load_expected_nets(expected_path)
    except ValueError as exc:
        return [str(exc)]

    netlist_text = netlist_path.read_text(encoding="utf-8")
    actual = parse_netlist_pins(netlist_text)

    for net_name, pins in expected_nets.items():
        want = set(pins)
        got = actual.get(net_name)
        if got is None:
            errors.append(f"net {net_name!r}: missing from netlist")
            continue
        missing = sorted(want - got)
        extra = sorted(got - want)
        if missing:
            errors.append(
                f"net {net_name!r}: missing pins: {', '.join(missing)}"
            )
        if extra:
            errors.append(
                f"net {net_name!r}: unexpected pins: {', '.join(extra)}"
            )
    return errors


def run_kicad(kicad_cli: Path, args: list[str], *, label: str) -> int:
    cmd = [str(kicad_cli), *args]
    print(f"  $ {' '.join(cmd)}")
    proc = subprocess.run(cmd, check=False)
    if proc.returncode not in (0, 5):
        print(
            f"error: {label} failed with unexpected exit {proc.returncode}",
            file=sys.stderr,
        )
    return proc.returncode


def check_board(kicad_cli: Path, board_dir: Path, report_dir: Path) -> bool:
    name = board_dir.name
    sch = board_dir / f"{name}.kicad_sch"
    pcb = board_dir / f"{name}.kicad_pcb"
    expected = board_dir / "expected-nets.yml"
    report_dir.mkdir(parents=True, exist_ok=True)

    erc_json = report_dir / "erc.json"
    drc_json = report_dir / "drc.json"
    netlist = report_dir / "netlist.net"

    print(f"==> {name}")

    if not expected.is_file():
        print(f"error: missing {expected}", file=sys.stderr)
        return False

    erc_rc = run_kicad(
        kicad_cli,
        [
            "sch",
            "erc",
            "--format",
            "json",
            "--severity-error",
            "--exit-code-violations",
            "-o",
            str(erc_json),
            str(sch),
        ],
        label="ERC",
    )
    if erc_rc == 5:
        print(f"error: ERC errors — see {erc_json}", file=sys.stderr)
        return False
    if erc_rc != 0:
        return False
    print(f"  ERC OK ({erc_json})")

    drc_rc = run_kicad(
        kicad_cli,
        [
            "pcb",
            "drc",
            "--format",
            "json",
            "--severity-error",
            "--schematic-parity",
            "--refill-zones",
            "--exit-code-violations",
            "-o",
            str(drc_json),
            str(pcb),
        ],
        label="DRC",
    )
    if drc_rc == 5:
        print(f"error: DRC errors — see {drc_json}", file=sys.stderr)
        return False
    if drc_rc != 0:
        return False
    print(f"  DRC OK ({drc_json})")

    net_rc = run_kicad(
        kicad_cli,
        ["sch", "export", "netlist", "-o", str(netlist), str(sch)],
        label="netlist export",
    )
    if net_rc != 0:
        return False

    net_errors = check_expected_nets(expected, netlist)
    if net_errors:
        print(f"error: expected-nets mismatch for {name}:", file=sys.stderr)
        for err in net_errors:
            print(f"  - {err}", file=sys.stderr)
        return False
    print(f"  expected-nets OK ({expected.name})")
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="KiCad ERC, DRC, and expected-nets for hardware/boards/"
    )
    parser.add_argument(
        "boards",
        nargs="*",
        help="Board directory names under hardware/boards/ (default: all)",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=None,
        help="Repository root (default: parent of scripts/)",
    )
    parser.add_argument(
        "--report-dir",
        type=Path,
        default=None,
        help="Write per-board reports here (default: temp dir, or PCB_REPORT_DIR)",
    )
    args = parser.parse_args(argv)

    repo_root = find_repo_root(args.repo_root)
    report_root = args.report_dir
    if report_root is None:
        env_dir = os.environ.get("PCB_REPORT_DIR")
        report_root = Path(env_dir) if env_dir else None

    kicad_cli = find_kicad_cli()
    print(f"kicad-cli: {kicad_cli}")
    ver = subprocess.run(
        [str(kicad_cli), "version"],
        check=False,
        capture_output=True,
        text=True,
    )
    if ver.stdout.strip():
        print(f"version: {ver.stdout.strip()}")

    boards = discover_boards(repo_root, args.boards or None)

    owned_temp: Path | None = None
    if report_root is None:
        owned_temp = Path(tempfile.mkdtemp(prefix="tiny-engineer-pcb-"))
        report_root = owned_temp
    else:
        report_root = report_root.resolve()
        report_root.mkdir(parents=True, exist_ok=True)

    print(f"reports: {report_root}")

    ok = True
    for board_dir in boards:
        if not check_board(kicad_cli, board_dir, report_root / board_dir.name):
            ok = False
            break

    if ok:
        print("All PCB checks passed.")
        if owned_temp is not None:
            shutil.rmtree(owned_temp, ignore_errors=True)
        return 0

    print("PCB checks failed.", file=sys.stderr)
    print(f"reports kept at: {report_root}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
