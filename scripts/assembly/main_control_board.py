#!/usr/bin/env python3
"""The main control board and the ESP32-C3-Zero's pin headers as coloured STLs,
for the 3D assembly viewer (scripts/assembly/hardware/).

The board comes from its KiCad design (hardware/boards/main-control-board/
main-control-board.kicad_pcb), read directly (no KiCad needed):

- the PCB from the Edge.Cuts outline (lines and arcs), its thickness from the
  stack-up, with the drilled mounting holes; red solder mask, FR4 edges;
- the copper pads (HASL), and the silkscreen's lines and polygons (its text is
  left out: the file keeps no outlines for it);
- each footprint's part where the footprint is: the five EasyEDA models of
  the project library (libraries/3d/easyeda.3dshapes/*.wrl: PCA9685, MAX98357A,
  TPS259571, USB-C, the 3 × 5 servo header) as they are, and the parts KiCad
  draws from its own library, which is not here, built from their footprint
  and data sheet sizes: the pin sockets and headers (KiCad's 2.54 mm models:
  socket 8.5 mm, header body 2.5 mm with 0.64 mm pins 6 mm above and 3 mm
  below it), the 10 × 10.5 mm electrolytic, the 1812 fuse, 0603 resistors,
  capacitors and LEDs, the JST SH and Molex PicoBlade connectors.

It is written in the frame of the design's own MainControlBoard (the
simplified board in TinyEngineer.f3d), so the extractor can swap it in: KiCad
x, y (y down) to (120 - y, 183.5 - x), z up from the back copper. The three
M2 mounting holes and the ESP32 socket rows land on the design's, which
extract.py checks.

The headers are two 1 × 9 male headers (2.54 mm, 2.5 mm body, 0.64 mm pins,
3 mm through the module, 6 mm into the sockets) in the ESP32-C3-Zero mesh's
frame (canfactory_board.ts): under the module, pins through its holes.

    python3 scripts/assembly/main_control_board.py

Each triangle's colour is in its attribute bytes, VisCAM style (bit 15, then
5 bits each of red, green and blue, sRGB), as build_glb.mjs reads them.
"""

from __future__ import annotations

import math
import re
import struct
import sys
from pathlib import Path

import numpy as np

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

# Pin headers and sockets, 2.54 mm (KiCad's models).
SOCKET_HEIGHT = 8.5
SOCKET_HOLE = 1.0
SOCKET_DEPTH = 7.0
HEADER_BODY = 2.5
HEADER_PIN = 0.64
HEADER_PIN_ABOVE = 6.0
HEADER_PIN_BELOW = 3.0

# The ESP32-C3-Zero's pins (extract.py, canfactory): two columns 15.24 apart, 9 at 2.54.
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


class Mesh:
    """Triangles with a colour each."""

    def __init__(self) -> None:
        self.triangles: list[np.ndarray] = []
        self.colors: list[tuple[float, float, float]] = []

    def add(self, corners: np.ndarray, color) -> None:
        corners = np.asarray(corners, float).reshape(-1, 3, 3)
        self.triangles.append(corners)
        self.colors += [color] * len(corners)

    def extend(self, other: "Mesh") -> None:
        self.triangles += other.triangles
        self.colors += other.colors

    def corners(self) -> np.ndarray:
        return np.concatenate(self.triangles) if self.triangles else np.zeros((0, 3, 3))

    def write(self, path: Path, transform=None, header: str = "") -> int:
        corners = self.corners()
        if transform is not None:
            corners = transform(corners.reshape(-1, 3)).reshape(-1, 3, 3)
        normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
        lengths = np.linalg.norm(normals, axis=1, keepdims=True)
        normals = np.divide(normals, lengths, out=np.zeros_like(normals), where=lengths > 0)
        record = np.dtype([("normal", "<f4", 3), ("vertices", "<f4", (3, 3)), ("attr", "<u2")])
        data = np.zeros(len(corners), dtype=record)
        data["normal"], data["vertices"] = normals, corners
        five = lambda c: min(31, max(0, round(c * 31)))  # noqa: E731  (the colours are sRGB)
        data["attr"] = [0x8000 | (five(r) << 10) | (five(g) << 5) | five(b) for r, g, b in self.colors]
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "wb") as handle:
            handle.write(header.encode("ascii")[:80].ljust(80, b"\0"))
            handle.write(struct.pack("<I", len(corners)))
            handle.write(data.tobytes())
        return len(corners)


def signed_area(points: np.ndarray) -> float:
    x, y = points[:, 0], points[:, 1]
    return 0.5 * float(np.dot(x, np.roll(y, -1)) - np.dot(np.roll(x, -1), y))


def triangulate(outer: np.ndarray, holes: list[np.ndarray] = ()) -> tuple[np.ndarray, np.ndarray]:
    """Ear clipping of a polygon (counter-clockwise) with holes, bridged in
    (Eberly): the points and triangles (counter-clockwise) over them."""
    outer = dedupe(outer)
    if len(outer) < 3 or abs(signed_area(outer)) < 1e-9:
        raise ValueError("triangulate: not a polygon")
    if signed_area(outer) < 0:
        outer = outer[::-1]
    if not len(holes) and convex(outer):
        return outer, np.array([(0, k, k + 1) for k in range(1, len(outer) - 1)], dtype=int)
    holes = [dedupe(h) for h in holes]
    ring = [tuple(p) for p in outer]
    pending = [np.asarray(h, float)[::-1] if signed_area(np.asarray(h, float)) > 0 else np.asarray(h, float) for h in holes]
    while pending:
        # The shortest bridge from any hole vertex to a ring vertex that crosses no edge.
        edges = [(np.array(ring[i]), np.array(ring[(i + 1) % len(ring)])) for i in range(len(ring))]
        for hole in pending:
            edges += [(hole[i], hole[(i + 1) % len(hole)]) for i in range(len(hole))]
        best = None
        R = np.array(ring)
        for k, hole in enumerate(pending):
            for h, m in enumerate(hole):
                for i in np.argsort(np.linalg.norm(R - m, axis=1))[:40]:
                    q = R[i]
                    if best is not None and np.linalg.norm(q - m) >= best[0]:
                        break
                    if not any(crosses(m, q, a, b) for a, b in edges):
                        best = (np.linalg.norm(q - m), k, h, int(i))
                        break
        if best is None:
            raise ValueError("triangulate: no bridge to a hole")
        _, k, h, i = best
        hole = pending.pop(k)
        hole_ring = [tuple(hole[(h + j) % len(hole)]) for j in range(len(hole) + 1)]
        ring = ring[: i + 1] + hole_ring + [ring[i]] + ring[i + 1 :]
    points = np.array(ring)
    index = list(range(len(points)))
    triangles = []
    guard = 0
    while len(index) > 3 and guard < 10 * len(points) ** 2:
        guard += 1
        n = len(index)
        for k in range(n):
            i0, i1, i2 = index[k - 1], index[k], index[(k + 1) % n]
            a, b, c = points[i0], points[i1], points[i2]
            if (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]) <= 1e-12:
                continue
            if any(point_in_triangle(points[j], a, b, c) and not np.allclose(points[j], a) and not np.allclose(points[j], b) and not np.allclose(points[j], c)
                   for j in index if j not in (i0, i1, i2)):
                continue
            triangles.append((i0, i1, i2))
            index.pop(k)
            break
        else:
            raise ValueError("triangulate: no ear (self-intersecting polygon?)")
    triangles.append(tuple(index))
    return points, np.array(triangles, dtype=int)


def dedupe(points) -> np.ndarray:
    """The polygon without repeated consecutive points (or a closing repeat of the first)."""
    points = np.asarray(points, float)
    keep = [p for i, p in enumerate(points) if not np.allclose(p, points[i - 1], atol=1e-7)]
    return np.array(keep if keep else points[:1])


def convex(points: np.ndarray) -> bool:
    """Whether a counter-clockwise polygon is convex (collinear points allowed)."""
    d = np.roll(points, -1, axis=0) - points
    cross = d[:, 0] * np.roll(d, -1, axis=0)[:, 1] - d[:, 1] * np.roll(d, -1, axis=0)[:, 0]
    return bool((cross >= -1e-9).all())


def crosses(p, q, a, b) -> bool:
    """Whether segment pq crosses segment ab in their interiors (shared ends do not count)."""
    if any(np.allclose(u, v) for u in (p, q) for v in (a, b)):
        return False
    orient = lambda u, v, w: (v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])  # noqa: E731
    d1, d2, d3, d4 = orient(a, b, p), orient(a, b, q), orient(p, q, a), orient(p, q, b)
    return d1 * d2 < 0 and d3 * d4 < 0


def point_in_triangle(p, a, b, c) -> bool:
    d1 = (p[0] - b[0]) * (a[1] - b[1]) - (a[0] - b[0]) * (p[1] - b[1])
    d2 = (p[0] - c[0]) * (b[1] - c[1]) - (b[0] - c[0]) * (p[1] - c[1])
    d3 = (p[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (p[1] - a[1])
    return not ((d1 < 0 or d2 < 0 or d3 < 0) and (d1 > 0 or d2 > 0 or d3 > 0))


def prism(outer, z0: float, z1: float, top, bottom=None, side=None, holes=()) -> Mesh:
    """A closed prism over a polygon (x, y; holes allowed) from z0 to z1, outward faces."""
    mesh = Mesh()
    outer = dedupe(outer)
    holes = [dedupe(h) for h in holes]
    points, triangles = triangulate(outer, holes)
    lift = lambda z: np.column_stack([points, np.full(len(points), z)])  # noqa: E731
    top_v, bottom_v = lift(z1), lift(z0)
    mesh.add(top_v[triangles], top)
    mesh.add(bottom_v[triangles[:, ::-1]], bottom or top)
    for loop, outward in [(np.asarray(outer, float), True), *[(np.asarray(h, float), False) for h in holes]]:
        ccw = signed_area(loop) > 0
        if ccw != outward:
            loop = loop[::-1]
        for a, b in zip(loop, np.roll(loop, -1, axis=0)):
            if np.allclose(a, b):
                continue
            quad = np.array([[*a, z0], [*b, z0], [*b, z1], [*a, z1]])
            mesh.add([quad[[0, 1, 2]], quad[[0, 2, 3]]], side or top)
    return mesh


def box(x0, x1, y0, y1, z0, z1, color, side=None) -> Mesh:
    return prism(np.array([[x0, y0], [x1, y0], [x1, y1], [x0, y1]]), z0, z1, color, side=side)


def frustum(cx, cy, z0, z1, half0, half1, color) -> Mesh:
    """A square frustum on z0..z1, half sizes half0 (bottom) and half1 (top): a pin's tip."""
    mesh = Mesh()
    b = np.array([[cx - half0, cy - half0, z0], [cx + half0, cy - half0, z0], [cx + half0, cy + half0, z0], [cx - half0, cy + half0, z0]])
    t = np.array([[cx - half1, cy - half1, z1], [cx + half1, cy - half1, z1], [cx + half1, cy + half1, z1], [cx - half1, cy + half1, z1]])
    mesh.add([b[[0, 2, 1]], b[[0, 3, 2]], t[[0, 1, 2]], t[[0, 2, 3]]], color)
    for i in range(4):
        j = (i + 1) % 4
        mesh.add([[b[i], b[j], t[j]], [b[i], t[j], t[i]]], color)
    return mesh


def circle(cx, cy, r, segments=24) -> np.ndarray:
    a = np.arange(segments) * 2 * math.pi / segments
    return np.column_stack([cx + r * np.cos(a), cy + r * np.sin(a)])


def rounded_rect(w, h, r, segments=4) -> np.ndarray:
    r = min(r, w / 2, h / 2)
    if r <= 1e-6:
        return np.array([[-w / 2, -h / 2], [w / 2, -h / 2], [w / 2, h / 2], [-w / 2, h / 2]])
    points = []
    for cx, cy, start in ((w / 2 - r, -h / 2 + r, -90), (w / 2 - r, h / 2 - r, 0), (-w / 2 + r, h / 2 - r, 90), (-w / 2 + r, -h / 2 + r, 180)):
        for k in range(segments + 1):
            a = math.radians(start + 90 * k / segments)
            points.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return np.array(points)


def arc_points(start, mid, end, step_deg: float = 10.0) -> np.ndarray:
    """Points along the arc through start, mid, end (inclusive)."""
    (x1, y1), (x2, y2), (x3, y3) = start, mid, end
    d = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2))
    ux = ((x1**2 + y1**2) * (y2 - y3) + (x2**2 + y2**2) * (y3 - y1) + (x3**2 + y3**2) * (y1 - y2)) / d
    uy = ((x1**2 + y1**2) * (x3 - x2) + (x2**2 + y2**2) * (x1 - x3) + (x3**2 + y3**2) * (x2 - x1)) / d
    r = math.hypot(x1 - ux, y1 - uy)
    a1, a2, a3 = (math.atan2(y - uy, x - ux) for x, y in (start, mid, end))
    sweep = (a3 - a1) % (2 * math.pi)
    if (a2 - a1) % (2 * math.pi) > sweep:
        sweep -= 2 * math.pi
    n = max(2, int(abs(math.degrees(sweep)) / step_deg) + 1)
    return np.array([(ux + r * math.cos(a1 + sweep * k / n), uy + r * math.sin(a1 + sweep * k / n)) for k in range(n + 1)])


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


def esp32_headers() -> Mesh:
    """Two 1 × 9 male headers under the ESP32-C3-Zero (its mesh's frame, PCB bottom at z = 0):
    the body below the module, the short pins up through its holes, the long ones down."""
    mesh = Mesh()
    cx, cy = ESP32_PIN_CENTRE
    for x in (cx - ESP32_ROW_SPACING / 2, cx + ESP32_ROW_SPACING / 2):
        for k in range(9):
            y = cy + (k - 4) * 2.54
            # Body on -2.5..0; pins 3 mm above it (through the module), 6 mm below it.
            mesh.extend(header_cell(x, y, -HEADER_BODY, BLACK, GOLD, up=HEADER_PIN_BELOW, down=HEADER_PIN_ABOVE))
    return mesh


def main() -> int:
    board = Board()
    n = board.build().write(OUT / "MainControlBoard.stl", to_design, "KiCad main-control-board, design frame")
    m = esp32_headers().write(OUT / "ESP32-C3-Zero-headers.stl", None, "2 x 1x9 male headers, ESP32-C3-Zero frame")
    print(f"{n} triangles -> {OUT / 'MainControlBoard.stl'}; {m} -> {OUT / 'ESP32-C3-Zero-headers.stl'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
