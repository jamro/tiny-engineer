#!/usr/bin/env python3
"""Compare a board production/bom.csv to the schematic, PCB footprints, and JLCPCB."""

from __future__ import annotations

import argparse
import csv
import json
import math
import re
import ssl
import sys
import urllib.error
import urllib.request
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

LCSC_FIELDS = (
    "LCSC Part #",
    "LCSC Part",
    "LCSC PN",
    "LCSC P/N",
    "LCSC Part No.",
    "LCSC Part Number",
    "JLCPCB Part #",
    "JLCPCB Part",
    "JLCPCB PN",
    "JLCPCB P/N",
    "JLCPCB Part No.",
    "JLCPCB Part Number",
)
BOM_COLUMNS = ("Designator", "Footprint", "Quantity", "Value", "LCSC Part #")
DETAIL_URL = (
    "https://cart.jlcpcb.com/shoppingCart/smtGood/getComponentDetail"
    "?componentCode={code}"
)
PASSIVE_ATTRS = ("Resistance", "Capacitance", "Inductance")
PIN_ATTRS = {
    "number of pins",
    "pin count",
    "pins",
    "number of contacts",
    "number of positions",
}
GENERIC_PACKAGES = {
    "smd",
    "tht",
    "plugin",
    "dip",
    "radial",
    "axial",
    "chip",
    "connector",
    "throughhole",
    "through-hole",
}
SI_MULT = {
    "p": 1e-12,
    "n": 1e-9,
    "u": 1e-6,
    "m": 1e-3,
    "k": 1e3,
    "K": 1e3,
    "M": 1e6,
    "G": 1e9,
}
LCSC_ID = re.compile(r"C\d+$", re.IGNORECASE)
SI_VALUE = re.compile(
    r"(\d+\.?\d*|\.\d+)\s*([pnumkKMG])?\s*([rRfFhH])?"
)
EURO_VALUE = re.compile(r"(\d+)([rRkKmM])(\d+)")
PIN_COUNT = re.compile(r"(\d+)")


@dataclass
class Finding:
    level: str
    refs: tuple[str, ...]
    message: str


@dataclass
class SchPart:
    ref: str
    value: str
    footprint: str
    lcsc: str
    lcsc_values: set[str] = field(default_factory=set)
    in_bom: bool = True
    dnp: bool = False
    lib_id: str = ""
    conflict: bool = False


@dataclass
class PcbPart:
    ref: str
    footprint: str


@dataclass
class BomRow:
    designators: list[str]
    footprint: str
    quantity: int
    value: str
    lcsc: str


@dataclass
class Catalog:
    data: dict | None = None
    missing: bool = False
    error: str | None = None


def die(message: str, code: int = 1) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(code)


def _is_repo_root(candidate: Path) -> bool:
    return (candidate / "hardware" / "boards").is_dir() and (
        (candidate / ".git").exists() or (candidate / "docs" / "pcb.md").is_file()
    )


def find_repo_root(start: Path) -> Path:
    searched: list[Path] = []
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
    if not list(board_dir.glob("*.kicad_pro")):
        die(f"no .kicad_pro in {board_dir}")
    return board_dir


def project_file(board_dir: Path, suffix: str) -> Path:
    preferred = board_dir / f"{board_dir.name}{suffix}"
    if preferred.is_file():
        return preferred
    matches = sorted(board_dir.glob(f"*{suffix}"))
    if not matches:
        die(f"no {suffix} in {board_dir}")
    return matches[0]


def tokenize(text: str) -> list[str]:
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
                if text[i] == "\\":
                    i += 1
                    if i < n:
                        buf.append(text[i])
                        i += 1
                    continue
                if text[i] == '"':
                    i += 1
                    break
                buf.append(text[i])
                i += 1
            tokens.append("".join(buf))
            continue
        start = i
        while i < n and not text[i].isspace() and text[i] not in "()":
            i += 1
        tokens.append(text[start:i])
    return tokens


def read_sexp(tokens: list[str], pos: int = 0):
    if pos >= len(tokens):
        raise ValueError("unexpected end of s-expression")
    if tokens[pos] != "(":
        return tokens[pos], pos + 1
    pos += 1
    items: list = []
    while pos < len(tokens) and tokens[pos] != ")":
        item, pos = read_sexp(tokens, pos)
        items.append(item)
    if pos >= len(tokens) or tokens[pos] != ")":
        raise ValueError("unclosed s-expression")
    return items, pos + 1


def parse_sexp_file(path: Path):
    tokens = tokenize(path.read_text(encoding="utf-8"))
    try:
        tree, pos = read_sexp(tokens, 0)
    except ValueError as exc:
        die(f"could not parse {path}: {exc}")
    if pos != len(tokens):
        die(f"could not parse {path}: trailing tokens")
    if not isinstance(tree, list):
        die(f"could not parse {path}: root is not a list")
    return tree


def properties(node: list) -> dict[str, list[str]]:
    found: dict[str, list[str]] = defaultdict(list)
    for child in node:
        if (
            isinstance(child, list)
            and len(child) >= 3
            and child[0] == "property"
        ):
            found[str(child[1])].append(str(child[2]))
    return found


def flag(node: list, name: str, default: bool) -> bool:
    for child in node:
        if isinstance(child, list) and len(child) >= 2 and child[0] == name:
            return str(child[1]).lower() == "yes"
    return default


def child_value(node: list, name: str) -> str:
    for child in node:
        if isinstance(child, list) and len(child) >= 2 and child[0] == name:
            return str(child[1])
    return ""


def is_placed_symbol(node: list) -> bool:
    return any(isinstance(child, list) and child and child[0] == "lib_id" for child in node)


def lcsc_from_props(props: dict[str, list[str]]) -> tuple[str, set[str]]:
    chosen = ""
    values: set[str] = set()
    for name in LCSC_FIELDS:
        for raw in props.get(name, []):
            value = raw.strip()
            if not value:
                continue
            values.add(value)
            if not chosen:
                chosen = value
    return chosen, values


def ref_sort_key(ref: str) -> tuple:
    match = re.match(r"([A-Za-z]+)(\d+)", ref)
    if match:
        return (match.group(1), int(match.group(2)), ref)
    return (ref, 0, ref)


def parse_si(text: str) -> float | None:
    cleaned = text.strip().replace("Ω", "").replace("µ", "u").replace("μ", "u")
    cleaned = re.sub(r"(?i)\s*(ohms|ohm|farads|farad|henrys|henry)s?\s*$", "", cleaned)
    cleaned = cleaned.strip()
    euro = EURO_VALUE.fullmatch(cleaned)
    if euro:
        prefix = euro.group(2)
        value = float(f"{euro.group(1)}.{euro.group(3)}")
        if prefix.lower() == "r":
            return value
        return value * SI_MULT[prefix]
    match = SI_VALUE.match(cleaned)
    if not match:
        return None
    rest = cleaned[match.end() :].strip()
    if rest and not re.match(r"(?i)[0-9.]*\s*v(olts?)?$", rest):
        return None
    value = float(match.group(1))
    prefix = match.group(2)
    if prefix:
        value *= SI_MULT[prefix]
    return value


def values_match(left: str, right: str) -> bool | None:
    a = parse_si(left)
    b = parse_si(right)
    if a is None or b is None:
        return None
    return math.isclose(a, b, rel_tol=0.02, abs_tol=1e-18)


def norm_package(text: str) -> str:
    text = text.lower().replace("μ", "u").replace("µ", "u")
    text = re.sub(r"[\s=_]+", "", text)
    text = text.replace("_", "").replace("-", "")
    text = re.sub(r"x0+(\d)", r"x\1", text)
    return text


def package_options(token: str) -> list[str]:
    options = [token]
    for suffix in ("mm", "mil", "inch"):
        if token.endswith(suffix) and len(token) > len(suffix) + 2:
            options.append(token[: -len(suffix)])
    expanded: list[str] = []
    for opt in options:
        expanded.append(opt)
        # JLCPCB "D10xL10.5mm" is the same body as KiCad "CP_Elec_10x10.5".
        collapsed = re.sub(
            r"[a-z]*(\d+(?:\.\d+)?)x[a-z]*(\d+(?:\.\d+)?)",
            r"\1x\2",
            opt,
        )
        if collapsed != opt:
            expanded.append(collapsed)
    return expanded


def package_matches(spec: str, footprints: list[str]) -> bool | None:
    blobs = [norm_package(fp) for fp in footprints if fp]
    if not spec.strip():
        return None
    if any(opt in blob for opt in package_options(norm_package(spec)) for blob in blobs):
        return True
    tokens = []
    for raw in re.split(r"[,;/]+", spec):
        if raw.strip().lower() in GENERIC_PACKAGES:
            continue
        token = norm_package(raw)
        if len(token) < 3 or token in GENERIC_PACKAGES:
            continue
        tokens.append(package_options(token))
    if not tokens:
        return None
    return all(any(any(opt in blob for blob in blobs) for opt in options) for options in tokens)


def footprint_contains(short: str, full: str) -> bool:
    if not short or not full:
        return False
    short_n = short.lower().replace(" ", "")
    full_n = full.lower()
    leaf = full_n.split(":")[-1]
    return short_n == leaf or short_n in leaf or short_n in full_n


def load_schematic(root: Path) -> dict[str, SchPart]:
    parts: dict[str, SchPart] = {}
    seen: set[Path] = set()

    def walk(path: Path) -> None:
        path = path.resolve()
        if path in seen:
            return
        if not path.is_file():
            die(f"schematic sheet not found: {path}")
        seen.add(path)
        tree = parse_sexp_file(path)
        if tree[0] != "kicad_sch":
            die(f"{path} is not a kicad_sch")
        for child in tree[1:]:
            if not isinstance(child, list) or not child:
                continue
            if child[0] == "symbol" and is_placed_symbol(child):
                part = part_from_symbol(child)
                if part is None:
                    continue
                existing = parts.get(part.ref)
                if existing is None:
                    parts[part.ref] = part
                    continue
                if (
                    existing.value != part.value
                    or existing.footprint != part.footprint
                    or existing.lcsc != part.lcsc
                    or existing.lcsc_values != part.lcsc_values
                ):
                    existing.lcsc_values |= part.lcsc_values
                    existing.conflict = True
                existing.dnp = existing.dnp or part.dnp
                existing.in_bom = existing.in_bom and part.in_bom
            elif child[0] == "sheet":
                sheet = ""
                for raw in properties(child).get("Sheetfile", []):
                    sheet = raw.strip()
                if sheet:
                    walk(path.parent / sheet)

    walk(root)
    return parts


def part_from_symbol(node: list) -> SchPart | None:
    props = properties(node)
    ref = (props.get("Reference") or [""])[-1].strip()
    lib_id = child_value(node, "lib_id")
    if not ref or ref.startswith("#") or lib_id.startswith("power:"):
        return None
    value = (props.get("Value") or [""])[-1].strip()
    footprint = (props.get("Footprint") or [""])[-1].strip()
    lcsc, values = lcsc_from_props(props)
    return SchPart(
        ref=ref,
        value=value,
        footprint=footprint,
        lcsc=lcsc,
        lcsc_values=values,
        in_bom=flag(node, "in_bom", True),
        dnp=flag(node, "dnp", False),
        lib_id=lib_id,
    )


def load_pcb(path: Path) -> dict[str, PcbPart]:
    tree = parse_sexp_file(path)
    if tree[0] != "kicad_pcb":
        die(f"{path} is not a kicad_pcb")
    parts: dict[str, PcbPart] = {}
    duplicates: set[str] = set()
    for child in tree[1:]:
        if not isinstance(child, list) or not child or child[0] != "footprint":
            continue
        if len(child) < 2 or isinstance(child[1], list):
            continue
        footprint = str(child[1]).strip()
        ref = (properties(child).get("Reference") or [""])[-1].strip()
        if not ref or ref.startswith("#"):
            continue
        if ref in parts:
            duplicates.add(ref)
            continue
        parts[ref] = PcbPart(ref=ref, footprint=footprint)
    for ref in duplicates:
        parts.pop(ref, None)
        parts[ref] = PcbPart(ref=ref, footprint="")
    return parts


def load_fp_libs(board_dir: Path) -> dict[str, Path]:
    table = board_dir / "fp-lib-table"
    if not table.is_file():
        return {}
    tree = parse_sexp_file(table)
    libs: dict[str, Path] = {}
    for child in tree[1:]:
        if not isinstance(child, list) or not child or child[0] != "lib":
            continue
        name = ""
        uri = ""
        for item in child[1:]:
            if isinstance(item, list) and len(item) >= 2 and item[0] == "name":
                name = str(item[1])
            elif isinstance(item, list) and len(item) >= 2 and item[0] == "uri":
                uri = str(item[1])
        if not name or not uri:
            continue
        expanded = uri.replace("${KIPRJMOD}", str(board_dir))
        libs[name] = Path(expanded)
    return libs


def project_footprint(board_dir: Path, libs: dict[str, Path], fp_id: str) -> Path | None:
    if ":" not in fp_id:
        return None
    lib, name = fp_id.split(":", 1)
    folder = libs.get(lib)
    if folder is None:
        return None
    path = folder / f"{name}.kicad_mod"
    if path.is_file() and board_dir in path.resolve().parents:
        return path
    return None


def count_pads(path: Path) -> int | None:
    tree = parse_sexp_file(path)
    count = 0

    def walk(node: list) -> None:
        nonlocal count
        for child in node:
            if not isinstance(child, list) or not child:
                continue
            if child[0] == "pad":
                kind = str(child[2]).lower() if len(child) > 2 else ""
                if kind not in {"npth", "np_thru_hole"}:
                    count += 1
                continue
            walk(child)

    walk(tree)
    return count


def load_bom(path: Path) -> list[BomRow]:
    if not path.is_file():
        die(
            f"{path} not found. Generate it with KiCad's Fabrication Toolkit "
            "(production/ is gitignored and is not in the repository)."
        )
    rows: list[BomRow] = []
    with path.open(newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        missing = [name for name in BOM_COLUMNS if reader.fieldnames is None or name not in reader.fieldnames]
        if missing:
            die(f"{path} is missing columns: {', '.join(missing)}")
        for raw in reader:
            designators = [
                item.strip()
                for item in (raw.get("Designator") or "").split(",")
                if item.strip()
            ]
            qty_text = (raw.get("Quantity") or "").strip()
            try:
                quantity = int(float(qty_text)) if qty_text else 0
            except ValueError:
                quantity = -1
            rows.append(
                BomRow(
                    designators=designators,
                    footprint=(raw.get("Footprint") or "").strip(),
                    quantity=quantity,
                    value=(raw.get("Value") or "").strip(),
                    lcsc=(raw.get("LCSC Part #") or "").strip(),
                )
            )
    return rows


def fetch_catalog(code: str) -> Catalog:
    url = DETAIL_URL.format(code=code.upper())
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/json",
            "User-Agent": "tiny-engineer-bom-check/1.0",
            "Referer": "https://jlcpcb.com/",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=30, context=ssl.create_default_context()) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return Catalog(missing=True)
        return Catalog(error=f"HTTP {exc.code}")
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError) as exc:
        return Catalog(error=str(exc))
    if not isinstance(payload, dict):
        return Catalog(error="unexpected JLCPCB payload")
    data = payload.get("data")
    if payload.get("code") == 200 and isinstance(data, dict):
        return Catalog(data=data)
    # Observed not-found / bad componentCode: business code 400, data null
    if payload.get("code") == 400 and data is None:
        return Catalog(missing=True)
    msg = payload.get("message") or "unexpected response"
    return Catalog(error=f"JLCPCB code {payload.get('code')}: {msg}")


def attr_map(data: dict) -> dict[str, str]:
    found: dict[str, str] = {}
    for item in data.get("attributes") or []:
        if not isinstance(item, dict):
            continue
        name = str(item.get("attribute_name_en") or "").strip()
        value = str(item.get("attribute_value_name") or "").strip()
        if name and value and name.lower() not in found:
            found[name.lower()] = value
    return found


def pin_count(attrs: dict[str, str]) -> int | None:
    for name, value in attrs.items():
        if name.lower() not in PIN_ATTRS:
            continue
        match = PIN_COUNT.search(value)
        if match:
            return int(match.group(1))
    return None


def model_matches(value: str, model: str) -> bool:
    left = re.sub(r"\s+", "", value.lower())
    right = re.sub(r"\s+", "", model.lower())
    if not left or not right:
        return False
    return right in left or left in right


def add(findings: list[Finding], level: str, refs: list[str] | tuple[str, ...], message: str) -> None:
    ordered = tuple(sorted({ref for ref in refs if ref}, key=ref_sort_key))
    findings.append(Finding(level, ordered, message))


def join_refs(refs: tuple[str, ...]) -> str:
    return ",".join(refs) if refs else "-"


def check(board_dir: Path) -> list[Finding]:
    sch_path = project_file(board_dir, ".kicad_sch")
    pcb_path = project_file(board_dir, ".kicad_pcb")
    bom_path = board_dir / "production" / "bom.csv"
    schematic = load_schematic(sch_path)
    board = load_pcb(pcb_path)
    bom = load_bom(bom_path)
    libs = load_fp_libs(board_dir)
    findings: list[Finding] = []

    bom_refs: dict[str, BomRow] = {}
    for row in bom:
        if row.quantity != len(row.designators):
            add(
                findings,
                "FAIL",
                row.designators,
                f"quantity {row.quantity} does not match {len(row.designators)} designators",
            )
        for ref in row.designators:
            if ref in bom_refs:
                add(findings, "FAIL", [ref], "designator repeated in bom.csv")
            bom_refs[ref] = row

    for ref, part in schematic.items():
        if part.conflict:
            add(findings, "FAIL", [ref], "symbol units disagree on value, footprint, or LCSC")
        if len(part.lcsc_values) > 1:
            listed = ", ".join(sorted(part.lcsc_values))
            add(findings, "FAIL", [ref], f"LCSC fields disagree: {listed}")
        row = bom_refs.get(ref)
        if part.in_bom and not part.dnp and row is None:
            add(findings, "FAIL", [ref], "on schematic (in_bom) but missing from bom.csv")
        if row is not None and part.dnp:
            add(findings, "WARN", [ref], "DNP but present in bom.csv")
        if row is not None and not part.in_bom:
            add(findings, "FAIL", [ref], "excluded from BOM (in_bom no) but listed in bom.csv")

    for ref, row in bom_refs.items():
        part = schematic.get(ref)
        if part is None:
            add(findings, "FAIL", [ref], "in bom.csv but not on schematic")
            continue
        if part.dnp or not part.in_bom:
            continue
        if row.lcsc != part.lcsc:
            add(
                findings,
                "FAIL",
                [ref],
                f"BOM LCSC {row.lcsc or '—'} does not match schematic {part.lcsc or '—'}",
            )
        if row.value != part.value:
            add(
                findings,
                "FAIL",
                [ref],
                f"BOM value {row.value or '—'} does not match schematic {part.value or '—'}",
            )
        pcb = board.get(ref)
        if pcb is None or not pcb.footprint:
            add(findings, "FAIL", [ref], "schematic footprint is not on the board")
        else:
            if part.footprint != pcb.footprint:
                add(
                    findings,
                    "FAIL",
                    [ref],
                    f"schematic footprint {part.footprint or '—'} differs from board {pcb.footprint}",
                )
            if not footprint_contains(row.footprint, part.footprint):
                add(
                    findings,
                    "FAIL",
                    [ref],
                    f"BOM footprint {row.footprint or '—'} not in schematic footprint {part.footprint or '—'}",
                )
            if not footprint_contains(row.footprint, pcb.footprint):
                add(
                    findings,
                    "FAIL",
                    [ref],
                    f"BOM footprint {row.footprint or '—'} not in board footprint {pcb.footprint}",
                )
        if not part.lcsc:
            add(findings, "FAIL", [ref], "missing LCSC Part #")
        elif not LCSC_ID.fullmatch(part.lcsc):
            add(findings, "FAIL", [ref], f"LCSC {part.lcsc} is not an LCSC id")

    catalog_cache: dict[str, Catalog] = {}
    by_code: dict[str, list[str]] = defaultdict(list)
    for ref, row in bom_refs.items():
        part = schematic.get(ref)
        if part is None or part.dnp or not part.in_bom:
            continue
        code = part.lcsc.strip()
        if not LCSC_ID.fullmatch(code):
            continue
        by_code[code.upper()].append(ref)

    for code, refs in by_code.items():
        catalog_cache[code] = fetch_catalog(code)
        catalog = catalog_cache[code]
        if catalog.error:
            add(
                findings,
                "FAIL",
                refs,
                f"JLCPCB lookup failed ({catalog.error}); package and value not checked",
            )
            continue
        if catalog.missing or catalog.data is None:
            add(findings, "FAIL", refs, f"LCSC {code} not in JLCPCB catalog")
            continue
        data = catalog.data
        attrs = attr_map(data)
        spec = str(data.get("componentSpecificationEn") or "").strip()
        model = str(data.get("componentModelEn") or "").strip()
        library = str(data.get("componentLibraryType") or "").strip().lower()
        stock = data.get("stockCount")
        if library in {"extended", "expand"}:
            add(findings, "WARN", refs, f"{code} is an extended part ({library})")
        if stock == 0:
            add(findings, "WARN", refs, f"{code} stockCount is 0")

        passive_checked = False
        for attr in PASSIVE_ATTRS:
            raw = attrs.get(attr.lower())
            if not raw:
                continue
            passive_checked = True
            for ref in refs:
                part = schematic[ref]
                compared = values_match(part.value, raw)
                if compared is None:
                    add(
                        findings,
                        "WARN",
                        [ref],
                        f"could not compare value {part.value or '—'} to JLCPCB {attr} {raw}",
                    )
                elif not compared:
                    add(
                        findings,
                        "FAIL",
                        [ref],
                        f"value {part.value} does not match JLCPCB {attr} {raw}",
                    )
        if not passive_checked and model:
            for ref in refs:
                part = schematic[ref]
                if not model_matches(part.value, model):
                    add(
                        findings,
                        "WARN",
                        [ref],
                        f'schematic value "{part.value}" vs MPN {model}',
                    )

        pins = pin_count(attrs)
        for ref in refs:
            part = schematic[ref]
            pcb = board.get(ref)
            names = [part.footprint, bom_refs[ref].footprint, pcb.footprint if pcb else ""]
            matched = package_matches(spec, names)
            if matched is None:
                add(findings, "WARN", [ref], f"package {spec or '—'} not comparable to footprint")
            elif not matched:
                add(
                    findings,
                    "FAIL",
                    [ref],
                    f"package {spec or '—'} not in footprint {part.footprint or '—'}",
                )
            if pins is None or pcb is None or not pcb.footprint:
                continue
            mod = project_footprint(board_dir, libs, pcb.footprint)
            if mod is None:
                continue
            pads = count_pads(mod)
            if pads is not None and pads != pins:
                add(
                    findings,
                    "WARN",
                    [ref],
                    f"footprint {pcb.footprint.split(':')[-1]} has {pads} pads, JLCPCB pins {pins}",
                )
    return findings


def collapse(findings: list[Finding]) -> list[Finding]:
    grouped: dict[tuple[str, str], set[str]] = defaultdict(set)
    for finding in findings:
        grouped[(finding.level, finding.message)].update(finding.refs)
    collapsed = [
        Finding(level, tuple(sorted(refs, key=ref_sort_key)), message)
        for (level, message), refs in grouped.items()
    ]
    order = {"FAIL": 0, "WARN": 1}
    collapsed.sort(key=lambda item: (order.get(item.level, 9), ref_sort_key(item.refs[0] if item.refs else ""), item.message))
    return collapsed


def passed_rows(bom: list[BomRow], findings: list[Finding]) -> int:
    bad: set[str] = set()
    for finding in findings:
        if finding.level == "FAIL":
            bad.update(finding.refs)
    count = 0
    for row in bom:
        if row.designators and all(ref not in bad for ref in row.designators):
            count += 1
    return count


def print_report(board_dir: Path, findings: list[Finding], bom: list[BomRow]) -> int:
    collapsed = collapse(findings)
    fails = [item for item in collapsed if item.level == "FAIL"]
    warns = [item for item in collapsed if item.level == "WARN"]
    print(f"{board_dir.name}  production/bom.csv")
    for item in fails + warns:
        print(f"{item.level:<4}  {join_refs(item.refs)}  {item.message}")
    print(f"PASS  {passed_rows(bom, collapsed)}")
    return 1 if fails else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Verify a board production/bom.csv against the schematic and JLCPCB.")
    parser.add_argument("--board", required=True, help="Board name or hardware/boards/<name> path")
    args = parser.parse_args(argv)
    repo = find_repo_root(Path.cwd())
    board_dir = resolve_board_dir(repo, args.board)
    bom = load_bom(board_dir / "production" / "bom.csv")
    findings = check(board_dir)
    return print_report(board_dir, findings, bom)


if __name__ == "__main__":
    raise SystemExit(main())
