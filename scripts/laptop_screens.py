#!/usr/bin/env python3
"""Build the printable laptop screen insert PDFs (one PDF per servo preset).

Insert size and visible window are measured from the exported meshes, so every
preset under 3d_models/parts/ gets correctly sized inserts:

- LaptopScreen.stl: the plate. The insert is cut to the plate outline.
- LaptopCase.stl: the lid pocket the plate slides into, and the lip in front of
  it whose opening is the part of the insert you actually see.

Screen designs are plain-text files in 3d_models/laptop_screens/screens/*.screen
(format: 3d_models/laptop_screens/README.md). Standard library only.

    python3 scripts/laptop_screens.py           # rewrite the PDFs
    python3 scripts/laptop_screens.py --check   # fail if a PDF is stale
"""

import argparse
import json
import math
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTS = ROOT / "3d_models" / "parts"
SERVOS_JSON = ROOT / "3d_models" / "fusion" / "TinyEngineerTools" / "servos.json"
SCREENS_DIR = ROOT / "3d_models" / "laptop_screens" / "screens"
OUT_DIR = ROOT / "3d_models" / "laptop_screens"


def out_pdf(sid):
    return OUT_DIR / f"laptop-screens-{sid}.pdf"


WEB_DIR = ROOT / "web" / "screens"

PT_PER_MM = 72 / 25.4
A4_MM = (210.0, 297.0)
PRESET_ORDER = ["sg90", "fs0307", "es9051", "hd1370a"]  # docs order; others follow


class ScreensError(Exception):
    pass


# ---------------------------------------------------------------- geometry

def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    length = math.sqrt(dot(a, a))
    return (a[0] / length, a[1] / length, a[2] / length)


def read_stl(path):
    """Binary STL -> list of (unit normal, area, centroid, (v0, v1, v2))."""
    data = path.read_bytes()
    if data[:5] == b"solid" and b"facet" in data[:400]:
        raise ScreensError(f"{path}: ASCII STL not supported")
    (count,) = struct.unpack_from("<I", data, 80)
    tris = []
    for i in range(count):
        f = struct.unpack_from("<12f", data, 84 + i * 50)
        v = (f[3:6], f[6:9], f[9:12])
        c = cross(sub(v[1], v[0]), sub(v[2], v[0]))
        area = math.sqrt(dot(c, c)) / 2
        if area < 1e-9:
            continue
        centroid = tuple((v[0][k] + v[1][k] + v[2][k]) / 3 for k in range(3))
        tris.append((norm(c), area, centroid, v))
    return tris


def clusters(values, gap=0.3):
    """Group sorted (value, item) pairs whose values are within `gap`."""
    out = []
    for value, item in sorted(values, key=lambda p: p[0]):
        if out and value - out[-1][-1][0] <= gap:
            out[-1].append((value, item))
        else:
            out.append([(value, item)])
    return out


def covered_intervals(tris2d, fixed_axis, fixed, lo, hi):
    """Merged intervals along the free axis where a line crosses the 2D triangles."""
    free = 1 - fixed_axis
    spans = []
    for tri in tris2d:
        hits = []
        for a, b in ((tri[0], tri[1]), (tri[1], tri[2]), (tri[2], tri[0])):
            da, db = a[fixed_axis] - fixed, b[fixed_axis] - fixed
            if (da < 0) != (db < 0):
                t = da / (da - db)
                hits.append(a[free] + t * (b[free] - a[free]))
        if len(hits) >= 2:
            spans.append((min(hits), max(hits)))
    merged = []
    for a, b in sorted(spans):
        if merged and a <= merged[-1][1] + 1e-4:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    gaps, pos = [], lo
    for a, b in merged:
        if a > pos:
            gaps.append((pos, a))
        pos = max(pos, b)
    if pos < hi:
        gaps.append((pos, hi))
    return gaps


def measure_plate(path):
    pts = [p for t in read_stl(path) for p in t[3]]
    size = sorted(max(p[k] for p in pts) - min(p[k] for p in pts) for k in range(3))
    thickness, height, width = size  # the screen is landscape
    return width, height, thickness


def measure_lid(path):
    """Pocket and window of the LaptopCase lid, in lid coordinates (mm).

    s runs across the screen, t runs up the screen (away from the hinge), and
    u is the screen normal pointing at the viewer.
    """
    tris = read_stl(path)
    buckets = {}
    for n, area, _, _ in tris:
        if max(abs(c) for c in n) < 0.99:  # the lid is the big tilted slab
            key = tuple(round(c * 20) for c in n)
            buckets[key] = buckets.get(key, 0) + area
    if not buckets:
        raise ScreensError(f"{path}: no tilted lid faces found")
    key = max(buckets, key=buckets.get)
    guess = norm(tuple(k / 20 for k in key))
    acc = [0.0, 0.0, 0.0]
    for n, area, _, _ in tris:
        if abs(dot(n, guess)) > 0.995:
            sign = 1 if dot(n, guess) > 0 else -1
            for k in range(3):
                acc[k] += sign * n[k] * area
    u = norm(tuple(acc))
    lid = [t for t in tris if abs(dot(t[0], u)) > 0.995]
    rest = [t for t in tris if abs(dot(t[0], u)) <= 0.995]

    def centroid(group):
        total = sum(t[1] for t in group)
        return tuple(sum(t[2][k] * t[1] for t in group) / total for k in range(3))

    lid_c, base_c = centroid(lid), centroid(rest)
    if dot(u, sub(base_c, lid_c)) < 0:  # the screen faces the keyboard
        u = (-u[0], -u[1], -u[2])
    # The hinge axis is the model axis the lid normal has no component along.
    hinge_axis = min(range(3), key=lambda k: abs(u[k]))
    if abs(u[hinge_axis]) > 0.01:
        raise ScreensError(f"{path}: lid normal {u} is not perpendicular to a model axis")
    axis = tuple(1.0 if k == hinge_axis else 0.0 for k in range(3))
    t_dir = norm(cross(u, axis))
    pts = [p for t in lid for p in t[3]]
    top = max(pts, key=lambda p: dot(p, t_dir))
    bottom = min(pts, key=lambda p: dot(p, t_dir))
    if dot(sub(top, base_c), sub(top, base_c)) < dot(sub(bottom, base_c), sub(bottom, base_c)):
        t_dir = (-t_dir[0], -t_dir[1], -t_dir[2])
    s_dir = cross(t_dir, u)  # viewer's right

    facing = lambda sign: [(dot(t[2], u), t) for t in lid if dot(t[0], u) * sign > 0]
    back_levels = clusters(facing(-1))
    front_levels = clusters(facing(+1))
    if len(back_levels) < 2:
        raise ScreensError(f"{path}: expected a back wall and a lip in the lid")
    lip = [t for _, t in back_levels[-1]]
    lip_d = back_levels[-1][0][0]
    back_d = back_levels[0][0][0]
    floors = [c for c in front_levels if back_d < c[0][0] < lip_d]
    if not floors:
        raise ScreensError(f"{path}: no pocket floor between back wall and lip")
    floor = [t for _, t in floors[-1]]
    depth = lip_d - floors[-1][-1][0]

    to2d = lambda p: (dot(p, s_dir), dot(p, t_dir))
    floor_pts = [to2d(p) for t in floor for p in t[3]]
    pocket = (min(p[0] for p in floor_pts), max(p[0] for p in floor_pts),
              min(p[1] for p in floor_pts), max(p[1] for p in floor_pts))
    lip2d = [tuple(to2d(p) for p in t[3]) for t in lip]
    s_mid = (pocket[0] + pocket[1]) / 2 + 1e-3
    t_gaps = covered_intervals(lip2d, 0, s_mid, pocket[2], pocket[3])
    t_gaps = [g for g in t_gaps if pocket[2] < g[0] and g[1] < pocket[3]] or t_gaps
    if not t_gaps:
        raise ScreensError(f"{path}: no window found in the lid lip")
    t0, t1 = max(t_gaps, key=lambda g: g[1] - g[0])
    s_gaps = covered_intervals(lip2d, 1, (t0 + t1) / 2 + 1e-3, pocket[0], pocket[1])
    s0, s1 = next(((a, b) for a, b in s_gaps if a < s_mid < b), (None, None))
    if s0 is None:
        raise ScreensError(f"{path}: window does not cross the pocket centre")
    return {"pocket": pocket, "window": (s0, s1, t0, t1), "depth": depth}


def measure_preset(preset_dir):
    """Insert and window in insert coordinates: mm, origin top-left, y down."""
    width, height, thickness = measure_plate(preset_dir / "stl" / "LaptopScreen.stl")
    lid = measure_lid(preset_dir / "stl" / "LaptopCase.stl")
    p_s0, p_s1, p_t0, p_t1 = lid["pocket"]
    w_s0, w_s1, w_t0, w_t1 = lid["window"]
    # The plate floats in the pocket by `slack`; assume it sits centred.
    slack = ((p_s1 - p_s0) - width, (p_t1 - p_t0) - height)
    left = p_s0 + slack[0] / 2
    top = p_t1 - slack[1] / 2
    window = (w_s0 - left, top - w_t1, w_s1 - w_s0, w_t1 - w_t0)
    if (min(slack) < -0.01 or window[0] < 0 or window[1] < 0
            or window[0] + window[2] > width + 0.01 or window[1] + window[3] > height + 0.01):
        raise ScreensError(f"{preset_dir.name}: window {window} does not sit on the "
                           f"{width:.2f} x {height:.2f} mm plate (slack {slack})")
    return {
        "insert": (width, height),
        "window": window,
        "slack": max(slack),
        "spare_depth": lid["depth"] - thickness,
    }


def presets():
    names = {v["servo_id"]: k for k, v in json.loads(SERVOS_JSON.read_text()).items()}
    found = [p.name for p in PARTS.iterdir() if (p / "stl" / "LaptopScreen.stl").is_file()]
    order = sorted(found, key=lambda s: (PRESET_ORDER.index(s) if s in PRESET_ORDER else 99, s))
    return [(sid, names.get(sid, sid)) for sid in order]


# ---------------------------------------------------------------- screens

COLOR_RE = re.compile(r"#[0-9a-fA-F]{6}$")
MARKUP_RE = re.compile(r"\{([a-z][a-z0-9_-]*|/)\}")


def parse_color(text, where):
    if not COLOR_RE.match(text):
        raise ScreensError(f"{where}: expected #rrggbb, got {text!r}")
    return tuple(int(text[i:i + 2], 16) / 255 for i in (1, 3, 5))


def parse_screen(path):
    """`# key: value` headers, then text lines with {name}...{/} colour spans."""
    meta = {"title": path.stem, "background": "#000000", "foreground": "#ffffff",
            "align": "top-left", "outline": "none", "paper-dim": ""}
    palette, body = {}, []
    lines = path.read_text(encoding="utf-8").splitlines()
    while lines and lines[0].startswith("#"):
        key, _, value = lines.pop(0)[1:].partition(":")
        key, value = key.strip(), value.strip()
        if key.startswith("color "):
            name = key[6:].strip()
            if name == "/" or not MARKUP_RE.fullmatch("{" + name + "}"):
                raise ScreensError(f"{path.name}: bad colour name {name!r}")
            palette[name] = parse_color(value, f"{path.name} {key}")
        elif key in meta:
            meta[key] = value
        else:
            raise ScreensError(f"{path.name}: unknown header {key!r}")
    if lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    fg = parse_color(meta["foreground"], f"{path.name} foreground")
    for n, line in enumerate(lines, 1):
        runs, colour, pos = [], fg, 0
        for m in MARKUP_RE.finditer(line):
            if m.start() > pos:
                runs.append((line[pos:m.start()], colour))
            name = m.group(1)
            if name == "/":
                colour = fg
            elif name in palette:
                colour = palette[name]
            else:
                raise ScreensError(f"{path.name}:{n}: unknown colour {{{name}}}")
            pos = m.end()
        if pos < len(line):
            runs.append((line[pos:], colour))
        for text, _ in runs:
            try:
                text.encode("cp1252")
            except UnicodeEncodeError as err:
                raise ScreensError(f"{path.name}:{n}: character not printable in PDF base fonts") from err
        body.append(runs)
    if meta["align"] not in ("top-left", "center"):
        raise ScreensError(f"{path.name}: align must be top-left or center")
    if meta["outline"] not in ("none", "window"):
        raise ScreensError(f"{path.name}: outline must be none or window")
    bg = meta["background"] = parse_color(meta["background"], f"{path.name} background")
    meta["foreground"] = fg
    dim_names = meta["paper-dim"].split()
    for name in dim_names:
        if name not in palette:
            raise ScreensError(f"{path.name}: paper-dim names unknown colour {name!r}")
    meta["paper_dim"] = {palette[name] for name in dim_names}
    for runs in body:
        for text, rgb in runs:
            ratio = contrast(rgb, bg)
            if text.strip() and ratio < MIN_CONTRAST:
                raise ScreensError(f"{path.name}: {text.strip()!r} has contrast {ratio:.1f}:1 against the "
                                   f"background; needs {MIN_CONTRAST}:1 to survive greyscale printing")
    return {"meta": meta, "lines": body}


MIN_CONTRAST = 3.0


def luminance(rgb):
    lin = [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in rgb]
    return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2]


def contrast(a, b):
    la, lb = sorted((luminance(a), luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def paper_ink(rgb, meta):
    """Paper mode: colours listed in `paper-dim` print grey, everything else black."""
    return PAPER_DIM if rgb in meta["paper_dim"] else PAPER_TEXT


def to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def to_srgb(c):
    return 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def tinted_ink(rgb, meta):
    """Tinted mode: keep the hue, darken until it reads on pale coloured paper.

    Scaling linear RGB keeps the colour's chromaticity, so green stays green.
    Text the design treats as dim (`paper-dim`) is allowed to stay lighter.
    """
    target = TINT_DIM_LUM if rgb in meta["paper_dim"] else TINT_TEXT_LUM
    lum = luminance(rgb)
    if lum <= target:
        return rgb
    k = target / lum
    return tuple(round(to_srgb(to_linear(c) * k), 4) for c in rgb)


def load_screens():
    files = sorted(SCREENS_DIR.glob("*.screen"))
    if not files:
        raise ScreensError(f"no *.screen files in {SCREENS_DIR}")
    return [parse_screen(f) for f in files]


# ---------------------------------------------------------------- PDF

def pdf_str(text):
    raw = text.encode("cp1252")
    return "(" + raw.replace(b"\\", b"\\\\").replace(b"(", b"\\(").replace(b")", b"\\)").decode("latin-1") + ")"


def num(v):
    s = f"{v:.3f}".rstrip("0").rstrip(".")
    return s if s not in ("-0", "") else "0"


FONTS = {"F1": "Courier", "F2": "Helvetica", "F3": "Helvetica-Bold"}


class Page:
    """Drawing in mm with the origin top-left, emitted as PDF operators."""

    def __init__(self):
        self.ops = [f"{num(PT_PER_MM)} 0 0 {num(-PT_PER_MM)} 0 {num(A4_MM[1] * PT_PER_MM)} cm"]

    def colour(self, rgb, stroke=False):
        self.ops.append(" ".join(num(c) for c in rgb) + (" RG" if stroke else " rg"))

    def rect(self, x, y, w, h, fill=None, stroke=None, width=0.1, dash=None):
        if fill:
            self.colour(fill)
        if stroke:
            self.colour(stroke, stroke=True)
            self.ops.append(f"{num(width)} w " + (f"[{num(dash)} {num(dash)}] 0 d" if dash else "[] 0 d"))
        op = "B" if fill and stroke else "f" if fill else "S"
        self.ops.append(f"{num(x)} {num(y)} {num(w)} {num(h)} re {op}")

    def line(self, x0, y0, x1, y1, rgb=(0, 0, 0), width=0.1):
        self.colour(rgb, stroke=True)
        self.ops.append(f"{num(width)} w [] 0 d {num(x0)} {num(y0)} m {num(x1)} {num(y1)} l S")

    def text(self, x, y, s, size, font="F2", rgb=(0, 0, 0)):
        """Baseline at y (mm). Text matrix flips y back so glyphs are upright."""
        self.colour(rgb)
        self.ops.append(f"BT /{font} {num(size)} Tf 1 0 0 -1 {num(x)} {num(y)} Tm {pdf_str(s)} Tj ET")

    def clip(self, x, y, w, h):
        self.ops.append(f"q {num(x)} {num(y)} {num(w)} {num(h)} re W n")

    def unclip(self):
        self.ops.append("Q")

    def stream(self):
        return "\n".join(self.ops).encode("latin-1")


def write_pdf(pages, title):
    objs = []

    def add(body):
        objs.append(body)
        return len(objs)

    font_ids = {k: add(f"<< /Type /Font /Subtype /Type1 /BaseFont /{v} /Encoding /WinAnsiEncoding >>".encode())
                for k, v in FONTS.items()}
    fonts = " ".join(f"/{k} {v} 0 R" for k, v in font_ids.items())
    pages_id = len(objs) + 1 + 2 * len(pages)
    kids = []
    for page in pages:
        data = zlib.compress(page.stream(), 9)
        content = add(b"<< /Length %d /Filter /FlateDecode >>\nstream\n" % len(data) + data + b"\nendstream")
        kids.append(add((f"<< /Type /Page /Parent {pages_id} 0 R /MediaBox [0 0 {num(A4_MM[0] * PT_PER_MM)} "
                         f"{num(A4_MM[1] * PT_PER_MM)}] /Resources << /Font << {fonts} >> >> "
                         f"/Contents {content} 0 R >>").encode()))
    assert add(f"<< /Type /Pages /Kids [{' '.join(f'{k} 0 R' for k in kids)}] /Count {len(kids)} "
               f"/ViewerPreferences << /PrintScaling /None >> >>".encode()) == pages_id
    info = add(f"<< /Title {pdf_str(title)} /Producer (scripts/laptop_screens.py) >>".encode())
    catalog = add(f"<< /Type /Catalog /Pages {pages_id} 0 R "
                  f"/ViewerPreferences << /PrintScaling /None >> >>".encode())
    out = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
    offsets = []
    for i, body in enumerate(objs, 1):
        offsets.append(len(out))
        out += b"%d 0 obj\n" % i + body + b"\nendobj\n"
    xref = len(out)
    out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1)
    out += b"".join(b"%010d 00000 n \n" % o for o in offsets)
    out += (b"trailer\n<< /Size %d /Root %d 0 R /Info %d 0 R >>\nstartxref\n%d\n%%%%EOF\n"
            % (len(objs) + 1, catalog, info, xref))
    return bytes(out)


# ---------------------------------------------------------------- layout

MARGIN = 12.0
GAP_X = 4.0
LABEL_H = 4.0
GREY = (0.45, 0.45, 0.45)
CUT = (0.6, 0.6, 0.6)
TEXT_PAD = 0.6  # mm inside the window, on top of half the pocket slack
LINE_H = 1.2    # line height in em
CHAR_W = 0.6    # Courier advance in em
PAPER_TEXT = (0, 0, 0)
PAPER_DIM = (0.45, 0.45, 0.45)
# Relative luminance ceilings for tinted text: ~5:1 and ~2.5:1 against a
# mid-pastel paper (luminance 0.6), so text survives yellow, blue or pink stock.
TINT_TEXT_LUM = 0.08
TINT_DIM_LUM = 0.2
MODES = {
    "colour": ("Printed background",
               "Background and text in colour; a black-only printer prints them as greys. Most ink."),
    "tinted": ("No background, colour text",
               "Colour printer. Only the text prints, darkened to read on pale paper; the paper shows through."),
    "paper": ("No background, black text",
              "Any printer. Only the text prints, in black and grey; the paper shows through. Least ink."),
}
INKS = {"colour": lambda rgb, meta: rgb, "tinted": tinted_ink, "paper": paper_ink}


def draw_insert(page, x, y, geo, screen, mode):
    w, h = geo["insert"]
    wx, wy, ww, wh = geo["window"]
    meta = screen["meta"]
    if mode == "colour":
        page.rect(x, y, w, h, fill=meta["background"])
    pad = TEXT_PAD + geo["slack"] / 2
    ax, ay, aw, ah = x + wx + pad, y + wy + pad, ww - 2 * pad, wh - 2 * pad
    lines = screen["lines"]
    if lines:
        cols = max(sum(len(t) for t, _ in runs) for runs in lines) or 1
        size_mm = min(aw / (cols * CHAR_W), ah / (len(lines) * LINE_H))
        block_w = cols * CHAR_W * size_mm
        block_h = len(lines) * LINE_H * size_mm
        if meta["align"] == "center":
            ax += (aw - block_w) / 2
            ay += (ah - block_h) / 2
        page.clip(x, y, w, h)
        for i, runs in enumerate(lines):
            cx = ax
            base = ay + (i + 0.8) * LINE_H * size_mm
            for text, rgb in runs:
                if text.strip():
                    ink = INKS[mode](rgb, meta)
                    page.text(cx, base, text, size_mm, font="F1", rgb=ink)
                cx += len(text) * CHAR_W * size_mm
        page.unclip()
    if meta["outline"] == "window":
        page.rect(x + wx, y + wy, ww, wh, stroke=GREY, width=0.15, dash=0.8)
    page.rect(x, y, w, h, stroke=CUT, width=0.1)


def draw_scale_check(page, x, y):
    page.rect(x, y, 10, 10, stroke=(0, 0, 0), width=0.2)
    page.text(x + 1.2, y + 5.8, "10 mm", 2.4, rgb=GREY)
    rx = x + 16
    page.line(rx, y + 10, rx + 50, y + 10, width=0.2)
    for mm in range(51):
        tick = 3.0 if mm % 10 == 0 else 2.0 if mm % 5 == 0 else 1.2
        page.line(rx + mm, y + 10, rx + mm, y + 10 - tick, width=0.12)
    for mm in range(0, 51, 10):
        page.text(rx + mm - 0.8, y + 5.6, str(mm), 2.2, rgb=GREY)
    page.text(rx + 53, y + 4.4, "Square must measure 10 x 10 mm and the ruler 50 mm.", 2.6)
    page.text(rx + 53, y + 8.6, "If not, reprint at 100% / Actual size (scaling off).", 2.6)


def preset_pages(sid, name, geo, screens, mode):
    """One print style for one preset: every design at least once, spares fill the last page."""
    pages, start = [], 0
    while start < len(screens):
        page, slots = preset_page(sid, name, geo, screens, mode, start)
        pages.append(page)
        start += slots
    return pages


def preset_page(sid, name, geo, screens, mode, start):
    page = Page()
    w, h = geo["insert"]
    wx, wy, ww, wh = geo["window"]
    heading, use = MODES[mode]
    y = MARGIN + 6
    page.text(MARGIN, y, f"Tiny Engineer - laptop screen inserts: {heading}", 6.0, font="F3")
    y += 7.5
    page.text(MARGIN, y, f"Servo preset: {name}  ({sid})", 4.6, font="F3")
    page.text(A4_MM[0] - MARGIN - 52, y, "Print one page that suits your printer.", 2.8, rgb=GREY)
    y += 6
    page.text(MARGIN, y, use, 2.8, font="F3")
    y += 4.2
    page.text(MARGIN, y, (f"Insert {w:.1f} x {h:.1f} mm (cut on the grey line). Visible window "
                          f"{ww:.1f} x {wh:.1f} mm, {wx:.1f} mm from the left and {wy:.1f} mm from "
                          f"the top edge."), 2.8)
    y += 4.2
    page.text(MARGIN, y, ("Slide the paper insert, then a same-size piece of clear PET, into the lid "
                          "pocket in front of LaptopScreen."), 2.8)
    y += 4
    draw_scale_check(page, MARGIN, y)
    y += 16

    cols = max(1, int((A4_MM[0] - 2 * MARGIN + GAP_X) // (w + GAP_X)))
    rows = max(1, int((A4_MM[1] - MARGIN - y) // (h + LABEL_H + 2)))
    grid_w = cols * w + (cols - 1) * GAP_X
    x0 = (A4_MM[0] - grid_w) / 2
    for i in range(cols * rows):
        screen = screens[(start + i) % len(screens)]
        r, c = divmod(i, cols)
        ix, iy = x0 + c * (w + GAP_X), y + r * (h + LABEL_H + 2)
        draw_insert(page, ix, iy, geo, screen, mode)
        page.text(ix, iy + h + 2.8, f"{sid} - {screen['meta']['title']}", 2.4, rgb=GREY)
    return page, cols * rows


def web_data(report):
    """Sizes and stock designs for the web generator (web/screens/)."""
    sizes = [{"id": sid, "name": name,
              "insert": [round(v, 2) for v in geo["insert"]],
              "window": [round(v, 2) for v in geo["window"]],
              "slack": round(geo["slack"], 2)} for sid, name, geo in report]
    designs = [{"file": f.name, "source": f.read_text(encoding="utf-8")}
               for f in sorted(SCREENS_DIR.glob("*.screen"))]
    dump = lambda data: (json.dumps(data, indent=2) + "\n").encode()
    return {WEB_DIR / "presets.json": dump(sizes), WEB_DIR / "designs.json": dump(designs)}


def build():
    screens = load_screens()
    outputs, report = {}, []
    for sid, name in presets():
        geo = measure_preset(PARTS / sid)
        pages = [p for mode in MODES for p in preset_pages(sid, name, geo, screens, mode)]
        outputs[out_pdf(sid)] = write_pdf(pages, f"Tiny Engineer laptop screen inserts ({sid})")
        report.append((sid, name, geo))
    outputs.update(web_data(report))
    return outputs, report


FLATE_STREAM = re.compile(rb"<< /Length (\d+) /Filter /FlateDecode >>\nstream\n")


def pdf_content(data):
    """Objects with streams inflated and the xref dropped, so zlib builds compare equal."""
    body = data[:data.rindex(b"\nxref\n")]
    out, pos = bytearray(), 0
    for m in FLATE_STREAM.finditer(body):
        if m.start() < pos:
            continue  # match inside compressed bytes already consumed
        end = m.end() + int(m.group(1))
        out += body[pos:m.start()] + zlib.decompress(body[m.end():end])
        pos = end
    return bytes(out + body[pos:])


def xref_ok(data):
    """startxref finds the table, /Size matches it, and every entry points at its own object."""
    m = re.search(rb"startxref\n(\d+)\n%%EOF\n$", data)
    if not m:
        return False
    xref = int(m.group(1))
    head = re.match(rb"xref\n0 (\d+)\n0000000000 65535 f \n", data[xref:])
    if not head:
        return False
    count = int(head.group(1)) - 1
    entries = re.match(rb"(?:\d{10} 00000 n \n){%d}trailer\n<< /Size (\d+) " % count, data[xref + head.end():])
    if not entries or int(entries.group(1)) != count + 1:
        return False
    offsets = re.findall(rb"(\d{10}) 00000 n \n", entries.group(0))
    return all(data.startswith(b"%d 0 obj\n" % i, int(o)) for i, o in enumerate(offsets, 1))


def up_to_date(path, data):
    if not path.is_file():
        return False
    old = path.read_bytes()
    if path.suffix != ".pdf":
        return old == data
    try:
        return xref_ok(old) and pdf_content(old) == pdf_content(data)
    except (ValueError, zlib.error):
        return False


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the committed outputs are out of date")
    args = parser.parse_args(argv)
    try:
        outputs, report = build()
    except ScreensError as err:
        print(f"laptop_screens: {err}", file=sys.stderr)
        return 1
    for sid, _, geo in report:
        w, h = geo["insert"]
        wx, wy, ww, wh = geo["window"]
        print(f"{sid:8} insert {w:6.2f} x {h:5.2f} mm  window {ww:6.2f} x {wh:5.2f} mm at "
              f"({wx:.2f}, {wy:.2f})  slack {geo['slack']:.2f}  spare depth {geo['spare_depth']:.2f} mm")
    stale = 0
    for path in sorted(set(OUT_DIR.glob("laptop-screens*.pdf")) - set(outputs)):
        rel = path.relative_to(ROOT)
        if args.check:
            print(f"{rel} is no longer generated: run python3 scripts/laptop_screens.py", file=sys.stderr)
            stale = 1
        else:
            path.unlink()
            print(f"removed {rel}")
    for path, data in outputs.items():
        rel = path.relative_to(ROOT)
        if args.check:
            if not up_to_date(path, data):
                print(f"{rel} is out of date: run python3 scripts/laptop_screens.py", file=sys.stderr)
                stale = 1
            else:
                print(f"{rel} is up to date")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            print(f"wrote {rel}")
    return stale


if __name__ == "__main__":
    sys.exit(main())
