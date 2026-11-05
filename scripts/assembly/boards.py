#!/usr/bin/env python3
"""The electronics the assembly model draws but the CAD does not: the main
control board, the ESP32-C3-Zero and its pin headers, as coloured STLs
(scripts/assembly/hardware/). Numpy only (meshes.py).

The main control board comes from its KiCad design (hardware/boards/
main-control-board/main-control-board.kicad_pcb), read directly (no KiCad):

- the PCB from the Edge.Cuts outline (lines and arcs), its thickness from the
  stack-up, with the unplated drills; red solder mask, FR4 edges;
- the copper pads (HASL), and the silkscreen's lines and polygons (not its
  text: the file keeps no outlines for it);
- a part on every footprint: the five EasyEDA models of the project library
  (libraries/3d/easyeda.3dshapes/*.wrl: PCA9685, MAX98357A, TPS259571, USB-C,
  the 3 × 5 servo header) as they are; the parts KiCad draws from its own
  library, which is not here, from their footprint and data sheet sizes: the
  2.54 mm sockets (8.5 mm) and headers (2.5 mm body, 0.64 mm pins 6 mm above
  and 3 mm below it), the 10 × 10.5 mm electrolytic, the 1812 fuse, 0603
  resistors, capacitors and LEDs, the JST SH and Molex PicoBlade connectors.

It is written in the frame of the design's MainControlBoard (the simplified
board in TinyEngineer.f3d), so the extractor can swap it in: KiCad x, y
(y down) to (120 - y, 183.5 - x), z up from the back copper. The three M2
holes and the ESP32 socket rows land on the design's (extract.py checks).

The Waveshare ESP32-C3-Zero follows Waveshare's drawing: a 23.5 × 18 mm PCB,
1 mm thick, corners R1, two columns of nine drilled and castellated pins
1.38 mm in from the long edges at 2.54 mm, the USB-C receptacle overhanging
the short edge. The parts on it (chip, regulator, crystal, ceramic antenna,
BOOT and RESET buttons, the WS2812 LED) are placed and sized from
Waveshare's photographs: measure a module before a tight fit. Its frame:
component side up, PCB corner at the origin, pins along +y, USB-C toward +y.

The headers are two 1 × 9 male headers (2.54 mm, 2.5 mm body, 0.64 mm pins,
3 mm through the module, 6 mm into the sockets) in the module's frame.

    python3 scripts/assembly/boards.py
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from meshes import Mesh, along_y, arc_points, box, circle, frustum, prism, rounded_rect  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
BOARD_DIR = ROOT / "hardware" / "boards" / "main-control-board"
PCB = BOARD_DIR / "main-control-board.kicad_pcb"
MODELS = BOARD_DIR / "libraries" / "3d" / "easyeda.3dshapes"
OUT = ROOT / "scripts" / "assembly" / "hardware"
#: KiCad (x, y down) to the design's MainControlBoard frame: (120 - y, 183.5 - x).
DESIGN_ORIGIN = (120.0, 183.5)
#: KiCad VRML models are in 0.1 inch.
VRML_UNIT = 2.54

MASK = (0.78, 0.07, 0.16)
FR4 = (0.86, 0.80, 0.62)
HASL = (0.80, 0.80, 0.82)
SILK = (0.95, 0.95, 0.93)
BLACK = (0.10, 0.10, 0.11)
GOLD = (0.85, 0.68, 0.25)
ALUMINIUM = (0.78, 0.79, 0.80)
NATURAL = (0.92, 0.87, 0.76)
ESP32_BLUE = (0.13, 0.32, 0.61)
PACKAGE_GREY = (0.23, 0.26, 0.29)
ANTENNA_RED = (0.77, 0.29, 0.26)
BUTTON_WHITE = (0.94, 0.94, 0.90)
PLUNGER = (0.90, 0.81, 0.61)
LED_LENS = (1.0, 0.97, 0.90)

# Pin headers and sockets, 2.54 mm (KiCad's models).
SOCKET_HEIGHT = 8.5
SOCKET_HOLE = 1.0
SOCKET_DEPTH = 7.0
HEADER_BODY = 2.5
HEADER_PIN = 0.64
HEADER_PIN_ABOVE = 6.0
HEADER_PIN_BELOW = 3.0

# The ESP32-C3-Zero (Waveshare): PCB, and its pins: two columns 15.24 apart, 9 at 2.54.
ESP32_SIZE = (18.0, 23.5, 1.0)
ESP32_PIN_CENTRE = (9.0, 11.75)
ESP32_ROW_SPACING = 15.24


# -- s-expressions -------------------------------------------------------------------------


def parse(text: str) -> list:
    stack: list[list] = [[]]
    for token in re.findall(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()]+', text):
        if token == "(":
            stack.append([])
        elif token == ")":
            node = stack.pop()
            stack[-1].append(node)
        else:
            stack[-1].append(token[1:-1] if token.startswith('"') else token)
    return stack[0][0]


def children(node: list, name: str) -> list[list]:
    return [c for c in node if isinstance(c, list) and c and c[0] == name]


def child(node: list, name: str) -> list | None:
    found = children(node, name)
    return found[0] if found else None


def numbers(node: list | None) -> list[float]:
    return [float(v) for v in node[1:] if not isinstance(v, list)] if node else []


# -- meshes --------------------------------------------------------------------------------


# -- the board -----------------------------------------------------------------------------


class Board:
    """The KiCad board in KiCad's 3D frame: x right, y up (KiCad's y negated), z up from the back copper."""

    def __init__(self, path: Path = PCB) -> None:
        self.pcb = parse(path.read_text())
        setup = child(self.pcb, "general")
        self.thickness = numbers(child(setup, "thickness"))[0]
        self.footprints = children(self.pcb, "footprint")

    @staticmethod
    def p(x: float, y: float) -> np.ndarray:
        return np.array([x, -y])

    def outline(self) -> np.ndarray:
        """The Edge.Cuts loop, chained from its lines and arcs."""
        pieces = []
        for kind in ("gr_line", "gr_arc"):
            for g in children(self.pcb, kind):
                if child(g, "layer")[1] != "Edge.Cuts":
                    continue
                s, e = numbers(child(g, "start")), numbers(child(g, "end"))
                pts = arc_points(s, numbers(child(g, "mid")), e) if kind == "gr_arc" else np.array([s, e])
                pieces.append(pts)
        loop = list(pieces.pop(0))
        while pieces:
            end = loop[-1]
            for k, piece in enumerate(pieces):
                if np.allclose(piece[0], end, atol=1e-4):
                    loop += list(piece[1:])
                elif np.allclose(piece[-1], end, atol=1e-4):
                    loop += list(piece[::-1][1:])
                else:
                    continue
                pieces.pop(k)
                break
            else:
                raise ValueError("Edge.Cuts is not one closed loop")
        loop = np.array(loop[:-1]) if np.allclose(loop[0], loop[-1], atol=1e-4) else np.array(loop)
        return np.column_stack([loop[:, 0], -loop[:, 1]])

    @staticmethod
    def place(fp: list):
        at = numbers(child(fp, "at"))
        fx, fy, theta = at[0], at[1], (at[2] if len(at) > 2 else 0.0)
        c, s = math.cos(math.radians(theta)), math.sin(math.radians(theta))
        # KiCad: y down, angles counter-clockwise on screen.
        to_board = lambda x, y: np.array([fx + x * c + y * s, -(fy - x * s + y * c)])  # noqa: E731
        return to_board, theta

    @staticmethod
    def ref(fp: list) -> str:
        return next(p[2] for p in children(fp, "property") if p[1] == "Reference")

    @staticmethod
    def name(fp: list) -> str:
        return fp[1].split(":")[-1]

    def holes(self) -> list[np.ndarray]:
        """Unplated drills (the mounting holes, the USB-C pegs)."""
        holes = []
        for fp in self.footprints:
            to_board, _ = self.place(fp)
            for pad in children(fp, "pad"):
                if pad[2] == "np_thru_hole":
                    x, y = numbers(child(pad, "at"))[:2]
                    holes.append(circle(*to_board(x, y), numbers(child(pad, "drill"))[0] / 2))
        return holes

    def build(self) -> Mesh:
        mesh = Mesh()
        t = self.thickness
        mesh.extend(prism(self.outline(), 0.0, t, MASK, side=FR4, holes=self.holes()))
        mesh.extend(self.pads())
        mesh.extend(self.silkscreen())
        for fp in self.footprints:
            mesh.extend(self.part(fp))
        return mesh

    def pads(self) -> Mesh:
        mesh = Mesh()
        t = self.thickness
        for fp in self.footprints:
            to_board, theta = self.place(fp)
            for pad in children(fp, "pad"):
                if pad[2] == "np_thru_hole" or pad[3] == "custom":
                    continue
                at = numbers(child(pad, "at"))
                angle = at[2] if len(at) > 2 else theta
                w, h = numbers(child(pad, "size"))[:2]
                shape = pad[3]
                if shape == "circle":
                    local = circle(0, 0, w / 2, 16)
                elif shape == "oval":
                    local = rounded_rect(w, h, min(w, h) / 2, 6)
                elif shape == "roundrect":
                    local = rounded_rect(w, h, min(w, h) * numbers(child(pad, "roundrect_rratio"))[0], 3)
                else:
                    local = rounded_rect(w, h, 0)
                c, s = math.cos(math.radians(angle)), math.sin(math.radians(angle))
                # The pad's angle is the footprint's plus its own, on screen (y down).
                centre = to_board(*at[:2])
                outline = np.array([centre + np.array([x * c + y * s, x * s - y * c]) for x, y in local])
                layers = child(pad, "layers")[1:]
                sides = {"F": "F.Cu" in layers or "*.Cu" in layers, "B": "B.Cu" in layers or "*.Cu" in layers}
                if sides["F"]:
                    mesh.extend(prism(outline, t, t + 0.035, HASL))
                if sides["B"]:
                    mesh.extend(prism(outline, -0.035, 0.0, HASL))
        return mesh

    def silkscreen(self) -> Mesh:
        """Silkscreen lines and polygons (not text), 0.015 mm proud of the mask."""
        mesh = Mesh()
        t = self.thickness

        def stroke(a, b, width, layer):
            a, b = np.asarray(a), np.asarray(b)
            d = b - a
            length = np.linalg.norm(d)
            if length < 1e-6:
                return
            n = np.array([-d[1], d[0]]) / length * width / 2
            quad = np.array([a - n, b - n, b + n, a + n])
            z0, z1 = (t, t + 0.015) if layer.startswith("F") else (-0.015, 0.0)
            mesh.extend(prism(quad, z0, z1, SILK))

        for g in children(self.pcb, "gr_line"):
            layer = child(g, "layer")[1]
            if layer in ("F.SilkS", "B.SilkS"):
                width = numbers(child(child(g, "stroke"), "width"))[0]
                stroke(self.p(*numbers(child(g, "start"))), self.p(*numbers(child(g, "end"))), width, layer)
        for g in children(self.pcb, "gr_poly"):
            layer = child(g, "layer")[1]
            if layer not in ("F.SilkS", "B.SilkS"):
                continue
            pts = np.array([self.p(*numbers(xy)) for xy in children(child(g, "pts"), "xy")])
            if len(pts) < 3:
                continue
            z0, z1 = (t, t + 0.015) if layer.startswith("F") else (-0.015, 0.0)
            try:
                mesh.extend(prism(pts, z0, z1, SILK))
            except ValueError:
                continue
        for fp in self.footprints:
            to_board, _ = self.place(fp)
            for g in children(fp, "fp_line"):
                layer = child(g, "layer")[1]
                if layer in ("F.SilkS", "B.SilkS"):
                    width = numbers(child(child(g, "stroke"), "width"))[0]
                    stroke(to_board(*numbers(child(g, "start"))), to_board(*numbers(child(g, "end"))), width, layer)
        return mesh

    def fab_box(self, fp: list) -> tuple[np.ndarray, float, float]:
        """The part's body from its fab outline: corners on the board, and its local size."""
        points = []
        for kind in ("fp_line", "fp_rect"):
            for g in children(fp, kind):
                if child(g, "layer")[1] in ("F.Fab", "B.Fab"):
                    points += [numbers(child(g, "start")), numbers(child(g, "end"))]
        local = np.array(points)
        x0, y0 = local.min(0)
        x1, y1 = local.max(0)
        to_board, _ = self.place(fp)
        return np.array([to_board(x0, y0), to_board(x1, y0), to_board(x1, y1), to_board(x0, y1)]), x1 - x0, y1 - y0

    def part(self, fp: list) -> Mesh:
        name, t = self.name(fp), self.thickness
        back = child(fp, "layer")[1] == "B.Cu"
        model = child(fp, "model")
        to_board, theta = self.place(fp)
        if model and "KIPRJMOD" in model[1]:
            return vrml(MODELS / Path(model[1]).name, to_board(0, 0), theta, t)
        pads = [numbers(child(p, "at"))[:2] for p in children(fp, "pad") if p[2] == "thru_hole"]
        if name.startswith("PinSocket") or (name.startswith("PinHeader") and model and "PinSocket" in model[1]):
            return socket([to_board(*p) for p in pads], t, back)
        if name.startswith("PinHeader"):
            return header([to_board(*p) for p in pads], t)
        if name.startswith("CP_Elec"):
            return electrolytic(self, fp, t)
        size = SMD_PARTS.get(name.split("_")[0])
        if size is not None:
            length, width, height, body, ends = size
            return chip(to_board(0, 0), theta, length, width, height, t, body, ends)
        if name.startswith(("JST_SH", "Molex_PicoBlade")):
            corners, _, _ = self.fab_box(fp)
            height = 2.9 if name.startswith("JST_SH") else 3.0
            return prism(corners, t, t + height, NATURAL)
        return Mesh()


#: Chip parts: length, width, height (mm), body colour, end-cap colour.
SMD_PARTS = {
    "R": (1.6, 0.8, 0.45, BLACK, HASL),
    "C": (1.6, 0.8, 0.8, (0.72, 0.58, 0.40), HASL),
    "LED": (1.6, 0.8, 0.6, (0.95, 0.95, 0.88), HASL),
    "Fuse": (4.5, 3.2, 0.85, (0.20, 0.20, 0.22), HASL),
}


def chip(centre, theta, length, width, height, t, body, ends) -> Mesh:
    mesh = Mesh()
    c, s = math.cos(math.radians(theta)), math.sin(math.radians(theta))
    turn = lambda pts: np.array([centre + np.array([x * c - y * s, x * s + y * c]) for x, y in pts])  # noqa: E731
    cap = min(0.3, length * 0.18)
    rect = lambda x0, x1: turn([(x0, -width / 2), (x1, -width / 2), (x1, width / 2), (x0, width / 2)])  # noqa: E731
    mesh.extend(prism(rect(-length / 2 + cap, length / 2 - cap), t + 0.035, t + 0.035 + height, body))
    for x0, x1 in ((-length / 2, -length / 2 + cap), (length / 2 - cap, length / 2)):
        mesh.extend(prism(rect(x0, x1), t + 0.035, t + 0.035 + height, ends))
    return mesh


def electrolytic(board: Board, fp: list, t: float) -> Mesh:
    """CP_Elec_10x10.5: the black base plate from the fab outline (chamfered at +), the can on it."""
    mesh = Mesh()
    to_board, _ = board.place(fp)
    points = []
    for g in children(fp, "fp_line"):
        if child(g, "layer")[1] == "F.Fab":
            points.append(to_board(*numbers(child(g, "start"))))
    centre = to_board(0, 0)
    outline = np.array(sorted(points, key=lambda q: math.atan2(q[1] - centre[1], q[0] - centre[0])))
    _, unique = np.unique(outline.round(4), axis=0, return_index=True)
    outline = outline[np.sort(unique)]
    mesh.extend(prism(outline, t, t + 1.0, BLACK))
    mesh.extend(prism(circle(*centre, 5.0, 40), t + 1.0, t + 10.0, ALUMINIUM))
    mesh.extend(prism(circle(*centre, 4.6, 40), t + 10.0, t + 10.5, (0.62, 0.63, 0.65)))
    return mesh


def socket(pins: list[np.ndarray], t: float, back: bool) -> Mesh:
    """A 2.54 mm female socket over these pins: black body, a square hole per pin."""
    mesh = Mesh()
    p = np.array(pins)
    xs, ys = np.unique(p[:, 0].round(3)), np.unique(p[:, 1].round(3))
    z_base, z_top = (0.0, -SOCKET_HEIGHT) if back else (t, t + SOCKET_HEIGHT)
    lo, hi = sorted((z_base, z_top))
    floor = z_top + (SOCKET_DEPTH if back else -SOCKET_DEPTH)
    solid_lo, solid_hi = sorted((z_base, floor))
    pocket_lo, pocket_hi = sorted((floor, z_top))
    h = SOCKET_HOLE / 2
    for x in xs:
        for y in ys:
            x0, x1, y0, y1 = x - 1.27, x + 1.27, y - 1.27, y + 1.27
            mesh.extend(box(x0, x1, y0, y1, solid_lo, solid_hi, BLACK))
            for bx in ((x0, x - h, y0, y1), (x + h, x1, y0, y1), (x - h, x + h, y0, y - h), (x - h, x + h, y + h, y1)):
                mesh.extend(box(*bx, pocket_lo, pocket_hi, BLACK))
    return mesh


def header(pins: list[np.ndarray], t: float) -> Mesh:
    """A 2.54 mm male header over these pins on the front: black body, the pins through it and the board."""
    mesh = Mesh()
    for x, y in pins:
        mesh.extend(header_cell(x, y, t, BLACK, GOLD, down=HEADER_PIN_BELOW + t))
    return mesh


def header_cell(x, y, base, body, pin, up: float = HEADER_PIN_ABOVE, down: float = HEADER_PIN_BELOW, plastic: float = HEADER_BODY) -> Mesh:
    """One pin of a male header: its share of the body on base..base+plastic, and the pin."""
    mesh = Mesh()
    h = HEADER_PIN / 2
    x0, x1, y0, y1 = x - 1.27, x + 1.27, y - 1.27, y + 1.27
    for bx in ((x0, x - h, y0, y1), (x + h, x1, y0, y1), (x - h, x + h, y0, y - h), (x - h, x + h, y + h, y1)):
        mesh.extend(box(*bx, base, base + plastic, body))
    top, bottom = base + plastic + up, base - down
    tip = 0.4
    mesh.extend(box(x - h, x + h, y - h, y + h, bottom + tip, top - tip, pin))
    mesh.extend(frustum(x, y, top - tip, top, h, h * 0.4, pin))
    mesh.extend(frustum(x, y, bottom, bottom + tip, h * 0.4, h, pin))
    return mesh


def vrml(path: Path, centre, theta: float, t: float) -> Mesh:
    """An EasyEDA VRML model on the board's front: each Shape's faces in its diffuse colour."""
    mesh = Mesh()
    text = path.read_text()
    c, s = math.cos(math.radians(theta)), math.sin(math.radians(theta))
    for shape in text.split("Shape{")[1:]:
        color = tuple(float(v) for v in re.search(r"diffuseColor\s+([-\d.eE]+)\s+([-\d.eE]+)\s+([-\d.eE]+)", shape).groups())
        points = np.array([float(v) for v in re.search(r"point\s*\[([^\]]*)\]", shape).group(1).replace(",", " ").split()]).reshape(-1, 3) * VRML_UNIT
        index = [int(v) for v in re.search(r"coordIndex\s*\[([^\]]*)\]", shape).group(1).replace(",", " ").split()]
        faces, face = [], []
        for i in index:
            if i < 0:
                faces.append(face)
                face = []
            else:
                face.append(i)
        world = np.column_stack([centre[0] + points[:, 0] * c - points[:, 1] * s, centre[1] + points[:, 0] * s + points[:, 1] * c, t + points[:, 2]])
        tris = [world[[f[0], f[k], f[k + 1]]] for f in faces for k in range(1, len(f) - 1)]
        if tris:
            mesh.add(np.array(tris), color)
    return mesh


def to_design(points: np.ndarray) -> np.ndarray:
    """KiCad's 3D frame (x, y up, z) to the design's MainControlBoard frame: a turn about z."""
    x, y, z = points[:, 0], points[:, 1], points[:, 2]
    return np.column_stack([DESIGN_ORIGIN[0] + y, DESIGN_ORIGIN[1] - x, z])


def esp32_pins() -> list[tuple[float, float]]:
    """The module's 18 pin centres in its frame, column by column."""
    cx, cy = ESP32_PIN_CENTRE
    return [(x, cy + (k - 4) * 2.54) for x in (cx - ESP32_ROW_SPACING / 2, cx + ESP32_ROW_SPACING / 2) for k in range(9)]


def esp32_outline() -> np.ndarray:
    """The module's PCB outline, counter-clockwise: corners R1, a castellated half hole
    (r 0.5) in the long edge at every pin."""
    w, h, _ = ESP32_SIZE
    r, notch = 1.0, 0.5
    ys = sorted({y for _, y in esp32_pins()})
    corner = lambda cx, cy, start: [(cx + r * math.cos(math.radians(start + 90 * k / 6)), cy + r * math.sin(math.radians(start + 90 * k / 6))) for k in range(7)]  # noqa: E731
    bite = lambda x, y, start, sweep: [(x + notch * math.cos(math.radians(start + sweep * k / 8)), y + notch * math.sin(math.radians(start + sweep * k / 8))) for k in range(9)]  # noqa: E731
    points = corner(r, r, 180) + corner(w - r, r, 270)
    for y in ys:  # up the right edge: bites into the board (toward -x)
        points += bite(w, y, 270, -180)
    points += corner(w - r, h - r, 0) + corner(r, h - r, 90)
    for y in reversed(ys):  # down the left edge
        points += bite(0.0, y, 90, -180)
    return np.array(points)


def esp32_c3_zero() -> Mesh:
    """The Waveshare ESP32-C3-Zero in its own frame (module docstring)."""
    mesh = Mesh()
    _, _, t = ESP32_SIZE
    pins = esp32_pins()
    mesh.extend(prism(esp32_outline(), 0.0, t, ESP32_BLUE, holes=[circle(x, y, 0.5, 16) for x, y in pins]))
    for x, y in pins:
        mesh.extend(prism(circle(x, y, 0.85, 16), t, t + 0.04, GOLD, holes=[circle(x, y, 0.5, 16)]))
    top = t
    for (x0, x1, y0, y1, height), color in (
        ((5.9, 10.9, 8.02, 13.02, 0.9), PACKAGE_GREY),    # ESP32-C3FN4
        ((12.29, 15.19, 6.59, 8.19, 1.3), PACKAGE_GREY),  # 3.3 V regulator
        ((2.25, 4.75, 7.49, 9.49, 0.6), ALUMINIUM),       # crystal
        ((5.63, 12.23, 0.29, 1.99, 1.0), ANTENNA_RED),    # ceramic antenna
        ((2.06, 5.56, 10.68, 14.88, 1.5), BUTTON_WHITE),  # BOOT
        ((12.47, 15.97, 10.68, 14.88, 1.5), BUTTON_WHITE),  # RESET
        ((8.03, 9.83, 13.2, 15.2, 0.28), BUTTON_WHITE),   # WS2812 base
    ):
        mesh.extend(box(x0, x1, y0, y1, top, top + height, color))
    mesh.extend(box(8.03, 9.83, 13.53, 14.87, top + 0.28, top + 0.8, LED_LENS))
    for cx in (3.81, 14.22):  # the buttons' plungers, 2.4 × 3.2 mm, their tops 1.8 mm above the PCB
        mesh.extend(prism(rounded_rect(2.4, 3.2, 1.2, 8) + (cx, 12.78), top + 1.5, top + 1.8, PLUNGER))
    # USB-C receptacle: 8.94 × 3.2 mm, on the PCB, 1.58 mm past its end.
    stadium = np.array([(9.0 + x, -(top + 1.6) + y) for x, y in rounded_rect(8.94, 3.2, 1.6, 8)])
    mesh.extend(along_y(prism(stadium, 17.73, 25.08, ALUMINIUM)))
    return mesh


def esp32_headers() -> Mesh:
    """Two 1 × 9 male headers under the ESP32-C3-Zero (its frame, PCB bottom at z = 0):
    the body below the module, the short pins up through its holes, the long ones down."""
    mesh = Mesh()
    for x, y in esp32_pins():
        mesh.extend(header_cell(x, y, -HEADER_BODY, BLACK, GOLD, up=HEADER_PIN_BELOW, down=HEADER_PIN_ABOVE))
    return mesh


def outputs() -> dict:
    """File name -> (mesh maker, transform, STL header): what main() writes."""
    return {
        "MainControlBoard.stl": (lambda: Board().build(), to_design, "KiCad main-control-board, design frame"),
        "ESP32-C3-Zero.stl": (esp32_c3_zero, None, "Waveshare ESP32-C3-Zero"),
        "ESP32-C3-Zero-headers.stl": (esp32_headers, None, "2 x 1x9 male headers, ESP32-C3-Zero frame"),
    }


def main(out: Path = OUT) -> int:
    for name, (make, transform, header) in outputs().items():
        print(f"{make().write(out / name, transform, header)} triangles -> {out / name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
