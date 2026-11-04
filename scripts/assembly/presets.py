#!/usr/bin/env python3
"""The assembled robot at another servo preset, for the 3D viewer.

The Fusion design is saved at one preset (SG90, see extract.py). The other
presets' parts are exported beside it (``3d_models/parts/<preset>/``: STL for
printing, STEP in each component's own frame), and their servo is the
design's parametric servo at ``servos.json``'s sizes. Fusion places every
component by its joints, outward from the body servo, which never moves. So
this script places them again for another preset:

- each joint's frame is moved from the SG90 parts to the preset's, by the
  feature it sits on (below), and the servo's frames by the servo's own
  dimensions;
- the joints are then followed from the body servo, as Fusion does.

A frame on a part moves with the feature it is on, found in that part's STEP
export at both presets, in this order:

1. ``shift``: a part whose shape does not change (lamp, bell, mug) only
   moves, by one translation.
2. ``patch``: the features around the frame move rigidly: the translation
   that lands most of them exactly on the preset's features.
3. ``feature``: the frame is a vertex or circle centre; it maps by its
   coordinate levels and snaps onto the preset's feature of its kind.
4. ``midpoint``: the frame is halfway between two vertices.
5. ``coaxial``: the frame is on a circle's axis, at the same axial offset.
6. ``axes``, ``weak patch``: last resorts.

Where a joint's frame cannot be moved reliably (the chest's servo pack: its
servos sit in their pockets with clearance), a group of parts joined by exact
joints is placed by its contacts with the parts already placed, axis by axis:
coordinate levels that coincide in the SG90 robot coincide again.

Checked against the design history: ``TinyEngineer.f3d`` at 22afc31 is saved
at FS0307 and at 268ad4b at SG90. From either one this places every
component of the other to 0.001 mm (``test_presets.py``, when the history is
there). Every run checks each joint again on the result and lists the ones it
overrode; ``check_steps``, ``check_clipping`` and ``check_poses`` then check
the parts on the meshes as for SG90.

    python3 scripts/assembly/presets.py --preset fs0307 --base artifacts/assembly/sg90 --out artifacts/assembly/fs0307
"""

from __future__ import annotations

import argparse
import itertools
import json
import re
import shutil
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from extract import (  # noqa: E402
    DEFAULT_F3D,
    electronics,
    FORMAT,
    ROOT,
    column_major,
    lower_arms,
    write_binary_stl,
)
from f3d_assembly import Design, inverse, read_stl, to_mm  # noqa: E402

SERVOS_JSON = ROOT / "3d_models" / "fusion" / "TinyEngineerTools" / "servos.json"
PARTS = ROOT / "3d_models" / "parts"
#: Printed parts the design mirrors from another one (no STEP of their own).
MIRRORED = {"UpperArm(Mirror)": "UpperArm"}
#: Hardware that keeps its shape at every preset.
FIXED_HARDWARE = {"OLED", "MainControlBoard"}
#: A placed part's STL must take at least this share of its STEP vertices.
MIN_PRINT_FIT = 0.5


# -- the servo ---------------------------------------------------------------------------


def servo_params(preset: str) -> dict[str, float]:
    """A preset's servo sizes (mm) from servos.json, keyed without the ``servo_`` prefix."""
    for entry in json.loads(SERVOS_JSON.read_text()).values():
        if entry["servo_id"] == preset:
            return {k.removeprefix("servo_"): float(v.split()[0]) for k, v in entry.items() if k != "servo_id"}
    raise SystemExit(f"{SERVOS_JSON.name}: no preset {preset!r}")


def servo_levels(p: dict[str, float]) -> tuple[dict[str, float], dict[str, float], dict[str, float]]:
    """Named coordinate levels of the design's servo, per axis of its frame.

    The frame has the shaft on +Y through the origin and its top at y 0.6;
    the model adds 0.6 mm clearance to the body, depth and tab thickness and
    0.3 mm to the shaft. Checked against the design's servo at SG90 and FS0307.
    """
    right = p["shaft_position"] + 0.3
    left = right - (p["body_width"] + 0.6)
    housing_top = 0.6 - (p["shaft_length"] + 0.3)
    housing_bottom = housing_top - p["shaft_housing_height"]
    tab_top = housing_bottom - p["gearbox_height"]
    tab_bottom = tab_top - (p["mount_thickness"] + 0.6)
    depth = (p["body_depth"] + 0.6) / 2
    return (
        {"tab_left": left - p["mount_width"], "body_left": left, "housing_left": left + p["gearbox_padding"],
         "axis": 0.0, "body_right": right, "tab_right": right + p["mount_width"]},
        {"shaft_top": 0.6, "housing_top": housing_top, "housing_bottom": housing_bottom, "tab_top": tab_top,
         "tab_mid": (tab_top + tab_bottom) / 2, "tab_bottom": tab_bottom, "body_bottom": tab_bottom - p["body_height"]},
        {"back": -depth, "mid": 0.0, "front": depth},
    )


def servo_points(levels) -> dict[tuple[str, str, str], np.ndarray]:
    xs, ys, zs = levels
    return {(a, b, c): np.array([xs[a], ys[b], zs[c]]) for a, b, c in itertools.product(xs, ys, zs)}


def servo_transfer(point: np.ndarray, src, dst) -> np.ndarray:
    """A point on the servo: each coordinate at a named level takes that level at the other preset."""
    out = []
    for k in range(3):
        names = [n for n, v in src[k].items() if abs(v - point[k]) < 1e-3]
        out.append(dst[k][names[0]] if names else point[k])
    return np.array(out)


def servo_holes(p: dict[str, float]) -> list[tuple[float, float]]:
    """The tabs' screw holes (x, z) in the servo frame: on the real tab's centre line (the
    model's tab less its clearance), moved out by ``mount_hole_width_offset``. Measured on
    the design's servo at SG90."""
    xs, _, _ = servo_levels(p)
    out = p["mount_width"] / 2 + p["mount_hole_width_offset"]
    return [(xs["body_left"] + 0.3 - out, p["mount_hole_depth_offset"]), (xs["body_right"] - 0.3 + out, p["mount_hole_depth_offset"])]


def holed_block(x0: float, x1: float, y0: float, y1: float, z0: float, z1: float, centre: tuple[float, float], radius: float,
                segments: int = 32) -> tuple[list, list]:
    """A closed box with a round hole along y through (x, z) = `centre`, outward faces."""
    cx, cz = centre
    corners = [(x1, z1), (x0, z1), (x0, z0), (x1, z0)]
    angles = sorted({2 * np.pi * i / segments for i in range(segments)} | {np.arctan2(z - cz, x - cx) % (2 * np.pi) for x, z in corners})
    inner, outer = [], []
    for a in angles:
        u = np.array([np.cos(a), np.sin(a)])
        inner.append((cx + radius * u[0], cz + radius * u[1]))
        # where the ray from the centre leaves the box
        t = min(((x1 - cx) / u[0] if u[0] > 1e-12 else (x0 - cx) / u[0] if u[0] < -1e-12 else np.inf),
                ((z1 - cz) / u[1] if u[1] > 1e-12 else (z0 - cz) / u[1] if u[1] < -1e-12 else np.inf))
        outer.append((cx + t * u[0], cz + t * u[1]))
    n = len(angles)
    vertices = [[x, y, z] for y in (y0, y1) for x, z in (*outer, *inner)]
    ob, ib, ot, it = 0, n, 2 * n, 3 * n
    triangles = []
    for i in range(n):
        j = (i + 1) % n
        # angles grow from +x towards +z, so about y they turn clockwise: wind the caps to suit
        triangles += [[ot + i, it + j, ot + j], [ot + i, it + i, it + j]]  # top (+y)
        triangles += [[ob + i, ob + j, ib + j], [ob + i, ib + j, ib + i]]  # bottom (-y)
        triangles += [[ob + i, ot + j, ob + j], [ob + i, ot + i, ot + j]]  # sides
        triangles += [[ib + i, ib + j, it + j], [ib + i, it + j, it + i]]  # hole
    return vertices, triangles


def servo_mesh(p: dict[str, float], segments: int = 32) -> tuple[np.ndarray, np.ndarray]:
    """The design's servo at a preset's sizes, as closed blocks (body, tabs with their screw
    holes, gearbox, shaft housing) and the shaft."""
    (xs, ys, zs) = servo_levels(p)
    d = zs["front"]
    blocks = [
        (xs["body_left"], xs["body_right"], ys["body_bottom"], ys["tab_bottom"]),
        (xs["body_left"], xs["body_right"], ys["tab_bottom"], ys["tab_top"]),
        (xs["body_left"], xs["body_right"], ys["tab_top"], ys["housing_bottom"]),
        (xs["housing_left"], xs["body_right"], ys["housing_bottom"], ys["housing_top"]),
    ]
    vertices, triangles = [], []
    for (x0, x1), centre in zip(((xs["tab_left"], xs["body_left"]), (xs["body_right"], xs["tab_right"])), servo_holes(p)):
        v, t = holed_block(x0, x1, ys["tab_bottom"], ys["tab_top"], -d, d, centre, p["mount_hole_radius"], segments)
        triangles += [[len(vertices) + k for k in tri] for tri in t]
        vertices += v
    for x0, x1, y0, y1 in blocks:
        base = len(vertices)
        vertices += [[x, y, z] for x in (x0, x1) for y in (y0, y1) for z in (-d, d)]
        # corners: index = 4*x + 2*y + z (0 low, 1 high); outward faces
        for quad in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            a, b, c, e = (base + q for q in quad)
            triangles += [[a, b, c], [a, c, e]]
    radius = p["shaft_radius"] + 0.3
    y0, y1 = ys["housing_top"], ys["shaft_top"]
    base = len(vertices)
    ring = [(radius * np.cos(2 * np.pi * i / segments), radius * np.sin(2 * np.pi * i / segments)) for i in range(segments)]
    vertices += [[x, y0, z] for x, z in ring] + [[x, y1, z] for x, z in ring] + [[0, y0, 0], [0, y1, 0]]
    bottom, top = base + 2 * segments, base + 2 * segments + 1
    for i in range(segments):
        j = (i + 1) % segments
        a, b, c, e = base + i, base + j, base + segments + j, base + segments + i
        triangles += [[a, e, c], [a, c, b], [bottom, a, b], [top, c, e]]
    return np.array(vertices, dtype=float), np.array(triangles)


# -- part features -----------------------------------------------------------------------


def step_features(path: Path) -> tuple[list, list]:
    """A STEP part's vertices (kind ``'v'``) and circle centres (kind ``('c', radius)``),
    and every circle as (radius, centre, unit axis)."""
    text = "".join(path.read_text(encoding="latin-1").split())
    entities = dict(re.findall(r"#(\d+)=([^;]*);", text))
    point = lambda i: np.array([float(v) for v in re.search(r"\(([^()]*)\)\)$", entities[i]).group(1).split(",")])  # noqa: E731
    features, circles = [], []
    for entity in entities.values():
        if entity.startswith("VERTEX_POINT"):
            features.append(("v", np.round(point(re.findall(r"#(\d+)", entity)[-1]), 4)))
        elif entity.startswith("CIRCLE"):
            axis, radius = re.match(r"CIRCLE\('[^']*',#(\d+),([-\d.E+]+)", entity).groups()
            refs = re.findall(r"#(\d+)", entities[axis])
            centre, direction = np.round(point(refs[0]), 4), point(refs[1])
            features.append((("c", round(float(radius), 4)), centre))
            circles.append((round(float(radius), 4), centre, direction / np.linalg.norm(direction)))
    return features, circles


def levels(features) -> list[np.ndarray]:
    P = np.array([q for _, q in features])
    return [np.unique(P[:, k].round(3)) for k in range(3)]


def rigid_shift(src, dst):
    a = np.array([q for k, q in src if k == "v"])
    b = np.array([q for k, q in dst if k == "v"])
    if len(a) != len(b) or not len(a):
        return None
    d = b - a
    return d[0] if np.abs(d - d[0]).max() < 1e-3 else None


class PartMap:
    """Every translation that lands two or more source features on destination features
    of their kind, and which source features each one lands."""

    def __init__(self, src, dst, tol: float = 1e-3) -> None:
        kinds = {k: i for i, k in enumerate(sorted({str(k) for k, _ in src} | {str(k) for k, _ in dst}))}
        self.kinds = kinds
        self.tol = tol
        self.P = np.array([q for _, q in src], dtype=float)
        self.K = np.array([kinds[str(k)] for k, _ in src])
        Q = np.array([q for _, q in dst], dtype=float)
        KQ = np.array([kinds[str(k)] for k, _ in dst])
        self.keys = np.unique(self.encode(Q, KQ))
        pairs = [(Q[KQ == k][None, :, :] - self.P[self.K == k][:, None, :]).reshape(-1, 3) for k in np.unique(self.K) if (KQ == k).any()]
        if pairs:
            uniq, counts = np.unique(np.round(np.concatenate(pairs) / tol).astype(np.int64), axis=0, return_counts=True)
            self.shifts = uniq[counts >= 2] * tol
        else:
            self.shifts = np.zeros((0, 3))
        # A match on a rare kind of feature (a circle of one radius) says more than one on a vertex.
        counts = {k: int((KQ == k).sum()) for k in np.unique(KQ)}
        self.weight = np.array([1.0 / max(1, counts.get(k, 0)) for k in self.K])
        self.M = np.zeros((len(self.shifts), len(self.P)), dtype=bool)
        for start in range(0, len(self.shifts), 2000):
            chunk = self.shifts[start : start + 2000]
            keys = self.encode((self.P[None] + chunk[:, None]).reshape(-1, 3), np.tile(self.K, len(chunk)))
            index = np.clip(np.searchsorted(self.keys, keys), 0, len(self.keys) - 1)
            self.M[start : start + len(chunk)] = (self.keys[index] == keys).reshape(len(chunk), len(self.P))

    def encode(self, X, K):
        I = np.round(X / self.tol).astype(np.int64) + (1 << 20)
        return (((K.astype(np.int64) << 21 | I[:, 0]) << 21 | I[:, 1]) << 21) | I[:, 2]

    def shift_at(self, p, radius: float):
        """(shift, features it lands, features near p) for the patch within radius of p."""
        near = np.linalg.norm(self.P - p, axis=1) <= radius
        if not near.any() or not len(self.shifts):
            return None, 0, int(near.sum())
        score = self.M[:, near] @ self.weight[near]
        ties = np.nonzero(score >= score.max() - 1e-9)[0]
        best = ties[np.argmin(np.linalg.norm(self.shifts[ties], axis=1))]
        return np.round(self.shifts[best], 3), int(self.M[best, near].sum()), int(near.sum())


class Part:
    """One printed part at the source and the destination preset (STEP frame)."""

    def __init__(self, src: Path, dst: Path, mirror: bool) -> None:
        flip = np.array([1, 1, -1]) if mirror else np.ones(3)
        (self.src, circles), (self.dst, _) = step_features(src), step_features(dst)
        if mirror:
            self.src = [(k, q * flip) for k, q in self.src]
            self.dst = [(k, q * flip) for k, q in self.dst]
            circles = [(r, c * flip, d * flip) for r, c, d in circles]
        self.circles = circles
        self.shift = rigid_shift(self.src, self.dst)
        self.map = PartMap(self.src, self.dst)
        self.dst_by_kind: dict[int, list] = {}
        for k, q in self.dst:
            self.dst_by_kind.setdefault(self.map.kinds[str(k)], []).append(np.asarray(q, dtype=float))
        self.dst_by_kind_array = {k: np.array(v) for k, v in self.dst_by_kind.items()}
        self.src_levels, self.dst_levels = levels(self.src), levels(self.dst)
        self._good = None

    def patch(self, p, radius):
        return self.map.shift_at(p, radius)

    def point(self, p):
        """A feature point moved rigidly with its neighbours: (point or None, quality)."""
        if self.shift is not None:
            return p + self.shift, 1.0
        for radius in (6.0, 10.0):
            shift, n, of = self.patch(p, radius)
            if shift is not None and n >= 3 and n >= 0.6 * of:
                return p + shift, n / of
        return None, 0.0

    def feature(self, p, tol=2e-3):
        """The source feature at p, mapped by its coordinate levels' indices and snapped onto
        the destination's nearest feature of its kind."""
        P = np.array([q for _, q in self.src])
        d = np.abs(P - p).max(1)
        i = int(d.argmin())
        if d[i] > tol:
            return None
        kind, guess = self.src[i][0], []
        for k in range(3):
            Ls, Ld = self.src_levels[k], self.dst_levels[k]
            j, n = int(np.abs(Ls - P[i][k]).argmin()), len(Ls)
            guess.append(Ld[j] if len(Ld) == n or j < n / 2 else Ld[len(Ld) - (n - j)])
        Q = np.array([q for k, q in self.dst if k == kind])
        return Q[np.abs(Q - np.array(guess)).max(1).argmin()] if len(Q) else None

    def good(self):
        if self._good is None:
            pairs = [(q, self.point(q)) for _, q in self.src]
            pairs = [(a, b) for a, (b, quality) in pairs if b is not None and quality >= 0.6]
            self._good = (np.array([a for a, _ in pairs]).reshape(-1, 3), np.array([b for _, b in pairs]).reshape(-1, 3))
        return self._good

    def level(self, k, value, fallback):
        """Where coordinate level `value` on axis k goes: the most common destination of the
        well-moved features on it, else the fallback point's."""
        A, B = self.good()
        sel = np.abs(A[:, k] - value) < 1e-3 if len(A) else np.zeros(0, dtype=bool)
        values = list(np.round(B[sel, k], 3)) if sel.any() else ([round(float(fallback[k]), 3)] if fallback is not None else [])
        if not values:
            return None
        vals, counts = np.unique(values, return_counts=True)
        return float(vals[counts.argmax()])

    def midpoint(self, p):
        V = np.array([q for k, q in self.src if k == "v"])
        for i in range(len(V)):
            for j in np.nonzero(np.abs(V - (2 * p - V[i])).max(1) < 1e-3)[0]:
                if j <= i:
                    continue
                a, b = self.feature(V[i]), self.feature(V[j])
                out = [self.level(k, V[i][k], a) for k in range(3)], [self.level(k, V[j][k], b) for k in range(3)]
                if None not in out[0] and None not in out[1]:
                    return (np.array(out[0]) + np.array(out[1])) / 2
        return None

    def coaxial(self, p):
        found = []
        for radius, centre, axis in self.circles:
            v = p - centre
            along = v @ axis
            if np.linalg.norm(v - along * axis) < 1e-3 and abs(along) > 1e-3:
                found.append((abs(along), centre))
        for _, centre in sorted(found, key=lambda f: f[0]):
            moved, _ = self.point(centre)
            if moved is None:
                moved = self.feature(centre)
            if moved is not None:
                return moved + (p - centre)
        return None

    def axes(self, p, tol=2e-3):
        out = []
        for k in range(3):
            Ls, Ld = self.src_levels[k], self.dst_levels[k]
            d = np.abs(Ls - p[k])
            j = int(d.argmin())
            if d[j] > tol or len(Ls) != len(Ld):
                return None
            out.append(Ld[j])
        return np.array(out)

    def anchored(self, p, near_mm=0.1, radius=8.0):
        """For a frame within near_mm of a feature: the shift that lands that feature on one of the
        destination's features of its kind, chosen by how many neighbours it lands too (rare kinds
        count more). (shift, landed, neighbours, weighted share) or None."""
        P = self.map.P
        d = np.linalg.norm(P - p, axis=1)
        i = int(d.argmin())
        if d[i] > near_mm:
            return None
        kind = self.map.K[i]
        near = d <= radius
        best = None
        near_points, near_kinds = P[near], self.map.K[near]
        for g in self.dst_by_kind.get(kind, []):
            shift = g - P[i]
            moved = near_points + shift
            landed = np.zeros(len(moved), dtype=bool)
            for k in np.unique(near_kinds):
                targets = self.dst_by_kind_array.get(k)
                if targets is None:
                    continue
                sel = near_kinds == k
                landed[sel] = np.abs(moved[sel][:, None, :] - targets[None, :, :]).max(2).min(1) < 1e-3
            score = float(landed @ self.map.weight[near])
            if best is None or score > best[0] + 1e-9 or (abs(score - best[0]) <= 1e-9 and np.linalg.norm(shift) < np.linalg.norm(best[1])):
                best = (score, shift, int(landed.sum()), int(near.sum()))
        if best is None:
            return None
        return best[1], best[2], best[3], best[0] / float(self.map.weight[near].sum())

    def frame(self, p):
        """(moved point, method, quality 0..1) for a joint frame's origin on this part."""
        if self.shift is not None:
            return p + self.shift, "shift", 1.0
        found = self.anchored(p)
        if found is not None:
            shift, n, of, weighted = found
            if n >= 2 and weighted >= 0.4:
                return p + shift, f"anchored patch {n}/{of} ({weighted:.2f})", max(0.6, weighted)
        for radius, share in ((8.0, 0.4), (16.0, 0.6), (32.0, 0.7)):
            shift, n, of = self.patch(p, radius)
            if shift is not None and n >= 3 and n >= share * of:
                return p + shift, f"patch {n}/{of}", n / of
        for method, quality in ((self.feature, 0.6), (self.midpoint, 0.5), (self.coaxial, 0.5), (self.axes, 0.45)):
            q = method(p)
            if q is not None:
                return q, method.__name__, quality
        for radius in (8.0, 16.0, 32.0):
            shift, n, of = self.patch(p, radius)
            if shift is not None and n >= 2:
                return p + shift, f"weak patch {n}/{of}", 0.2 * n / of
        return None, "none", 0.0


# -- the solve ---------------------------------------------------------------------------


class Solver:
    """Places a design's components at another preset (see the module docstring)."""

    def __init__(self, design: Design | Path, src_preset: str, dst_preset: str, src_step: Path | None = None, dst_step: Path | None = None) -> None:
        self.design = design if isinstance(design, Design) else Design(design)
        self.occ = {o.instances[-1]: o for o in self.design.occurrences() if not o.in_print_layout and o.source != "identity"}
        self.src_step = Path(src_step or PARTS / src_preset / "step")
        self.dst_step = Path(dst_step or PARTS / dst_preset / "step")
        self.servo_src = servo_levels(servo_params(src_preset))
        self.servo_dst = servo_levels(servo_params(dst_preset))
        self.parts: dict[str, Part | None] = {}
        self.deltas: dict[str, np.ndarray] = {}
        self.log: list[str] = []

    def part(self, component: str) -> Part | None:
        if component not in self.parts:
            name = MIRRORED.get(component, component)
            src, dst = self.src_step / f"{name}.step", self.dst_step / f"{name}.step"
            self.parts[component] = Part(src, dst, component in MIRRORED) if src.exists() and dst.exists() else None
        return self.parts[component]

    def delta(self, component: str) -> np.ndarray:
        """STEP frame minus the design's component frame (a translation, by vote)."""
        if component not in self.deltas:
            part = self.part(component)
            best = (np.zeros(3), -1)
            if part is not None and component not in MIRRORED:
                S = np.array([q for k, q in part.src if k == "v"])
                keys = {tuple(np.round(q, 2)) for q in S}
                solids = [v for blob in self.design.component(component).bodies for v in self.design.solid_vertices(blob)]
                for V in sorted(solids, key=len, reverse=True)[:3]:
                    votes: dict[tuple, int] = {}
                    for v in V[:: max(1, len(V) // 25)]:
                        for t in np.round(S - v, 2):
                            votes[tuple(t)] = votes.get(tuple(t), 0) + 1
                    for t, _ in sorted(votes.items(), key=lambda kv: -kv[1])[:5]:
                        n = sum(tuple(np.round(v + np.array(t), 2)) in keys for v in V)
                        if n > best[1]:
                            best = (np.array(t), n)
            self.deltas[component] = best[0]
        return self.deltas[component]

    def transfer(self, inst: int, frame: np.ndarray):
        """A joint frame on an occurrence, at the destination preset: (frame, method, quality)."""
        component = self.occ[inst].component
        p = frame[:3, 3]
        if component == "Servo":
            q, how, quality = servo_transfer(p, self.servo_src, self.servo_dst), "servo", 1.0
        elif component in FIXED_HARDWARE:
            q, how, quality = p, "hardware", 1.0
        else:
            part = self.part(component)
            if part is None:
                return None, "no STEP", 0.0
            delta = self.delta(component)
            q, how, quality = part.frame(p + delta)
            if q is not None:
                q = q - delta
        if q is None:
            return None, how, 0.0
        out = frame.copy()
        out[:3, 3] = q
        return out, how, quality

    def joints(self):
        """(name, a, b, frame on a, frame on b, residual) for joints between placed
        occurrences, with occ[a] @ fa @ inv(fb) @ residual == occ[b] in the design."""
        result = []
        for joint in self.design.joints():
            ids = [r for r in joint.referenced if r in self.occ]
            if len(ids) < 2:
                continue
            frames = [to_mm(v) for v in joint.frames.values()]
            best = None
            for a, b in itertools.permutations(ids, 2):
                for fa, fb in itertools.permutations(frames, 2):
                    e = np.abs(self.occ[a].matrix @ fa - self.occ[b].matrix @ fb).max()
                    if best is None or e < best[0]:
                        best = (e, a, b, fa, fb)
            _, a, b, fa, fb = best
            result.append((joint.name, a, b, fa, fb, inverse(self.occ[a].matrix @ fa @ inverse(fb)) @ self.occ[b].matrix))
        return result

    def member_points(self, inst: int):
        """(source, destination) feature points of an occurrence, design frame, that move with confidence."""
        component = self.occ[inst].component
        if component == "Servo":
            src, dst = servo_points(self.servo_src), servo_points(self.servo_dst)
            return [(src[k], dst[k]) for k in src]
        part = None if component in FIXED_HARDWARE else self.part(component)
        if part is None:
            return []
        delta = self.delta(component)
        A, B = part.good()
        return [(a - delta, b - delta) for a, b in zip(A, B)]

    def cluster(self, members: dict, placed: dict, local: bool, min_votes: int = 2, near_mm: float = 0.5):
        """Translation of a rigid group (inst -> (source occurrence, destination occurrence
        relative to the group)) against the placed parts it touches, axis by axis: levels that
        coincide in the source robot coincide again; where none do on an axis, levels within
        near_mm keep their offset. (translation or None, votes per axis)."""
        data = []
        for inst, (src_occ, rel) in members.items():
            P = self.member_points(inst)
            if P:
                A, B = np.array([p for p, _ in P]), np.array([q for _, q in P])
                data.append((A @ src_occ[:3, :3].T + src_occ[:3, 3], B @ rel[:3, :3].T + rel[:3, 3]))
        if not data:
            return None, [0, 0, 0]
        L = next(iter(members.values()))[0][:3, :3] if local else np.eye(3)
        WB = np.concatenate([d[0] for d in data]) @ L
        RB = np.concatenate([d[1] for d in data]) @ L
        lo, hi = WB.min(0) - 1.0, WB.max(0) + 1.0
        WA, WA2 = [], []
        for x, matrix in placed.items():
            P = self.member_points(x)
            if not P:
                continue
            A, B = np.array([p for p, _ in P]), np.array([q for _, q in P])
            oa = self.occ[x].matrix
            W = (A @ oa[:3, :3].T + oa[:3, 3]) @ L
            keep = np.all((W >= lo) & (W <= hi), axis=1)
            if keep.any():
                WA.append(W[keep])
                WA2.append(((B @ matrix[:3, :3].T + matrix[:3, 3]) @ L)[keep])
        if not WA:
            return None, [0, 0, 0]
        WA, WA2 = np.concatenate(WA), np.concatenate(WA2)
        t, support = np.full(3, np.nan), [0, 0, 0]
        for w in range(3):
            for tolerance in (1e-3, near_mm):
                votes: dict[float, int] = {}
                for j in range(len(WB)):
                    for i in np.nonzero(np.abs(WA[:, w] - WB[j, w]) <= tolerance)[0]:
                        v = round(float(WA2[i, w] - RB[j, w] - (WA[i, w] - WB[j, w])), 3)
                        votes[v] = votes.get(v, 0) + 1
                if votes:
                    v, n = max(votes.items(), key=lambda kv: kv[1])
                    if n >= min_votes:
                        t[w], support[w] = v, n
                        break
        if np.isnan(t).any():
            return None, support
        return (L @ t if local else t), support

    def solve(self):
        """Occurrence matrices at the destination preset (instance -> 4x4), and every joint
        checked on them: (name, mismatch mm or None, method a, method b)."""
        roots = [i for i, o in self.occ.items() if o.source == "instance"]
        if len(roots) != 1:
            raise SystemExit(f"expected one occurrence placed by joints only (the body servo), found {len(roots)}")
        placed = {roots[0]: self.occ[roots[0]].matrix}
        joints = self.joints()
        moved = {}
        for name, a, b, fa, fb, _ in joints:
            fa2, ha, qa = self.transfer(a, fa)
            fb2, hb, qb = self.transfer(b, fb)
            if np.abs(self.occ[a].matrix - self.occ[b].matrix).max() < 1e-6 and np.abs(fa[:3, 3] - fb[:3, 3]).max() < 1e-6 and max(qa, qb) > 0:
                # One point in one coordinate system: both sides take the more reliable move.
                best = fa2 if qa >= qb else fb2
                fa2, fb2 = fa.copy(), fb.copy()
                fa2[:3, 3] = fb2[:3, 3] = best[:3, 3]
                ha = hb = f"shared {ha if qa >= qb else hb}"
                qa = qb = max(qa, qb)
            moved[name] = (fa2, fb2, ha, hb, min(qa, qb))
        strong = [j for j in joints if moved[j[0]][4] >= 0.9]

        def by_joints(threshold: float) -> bool:
            found: dict[int, list] = {}
            for name, a, b, fa, fb, E in joints:
                fa2, fb2, _, _, q = moved[name]
                if fa2 is None or fb2 is None or q < threshold:
                    continue
                if a in placed and b not in placed:
                    found.setdefault(b, []).append((q, name, placed[a] @ fa2 @ inverse(fb2) @ E))
                elif b in placed and a not in placed:
                    found.setdefault(a, []).append((q, name, placed[b] @ inverse(E) @ fb2 @ inverse(fa2)))
            for inst, options in found.items():
                q, name, matrix = max(options, key=lambda o: o[0])
                placed[inst] = matrix
                if threshold < 0.6:
                    self.log.append(f"{self.occ[inst].component}: joint {name!r} ({q:.2f})")
            return bool(found)

        def by_contacts(min_support: int) -> bool:
            options, seen = [], set()
            for seed in self.occ:
                if seed in placed or seed in seen:
                    continue
                rel = {seed: self.occ[seed].matrix.copy()}
                rel[seed][:3, 3] = 0.0
                grow = True
                while grow:
                    grow = False
                    for name, a, b, fa, fb, E in strong:
                        fa2, fb2 = moved[name][:2]
                        if a in rel and b not in rel and b not in placed:
                            rel[b], grow = rel[a] @ fa2 @ inverse(fb2) @ E, True
                        elif b in rel and a not in rel and a not in placed:
                            rel[a], grow = rel[b] @ inverse(E) @ fb2 @ inverse(fa2), True
                seen |= set(rel)
                members = {i: (self.occ[i].matrix, rel[i]) for i in rel}
                for local in (False, True):
                    t, support = self.cluster(members, placed, local)
                    if t is not None:
                        options.append((min(support), t, rel, support))
                        break
            options = [o for o in options if o[0] >= min_support]
            if not options:
                return False
            _, t, rel, support = max(options, key=lambda o: o[0])
            origin = np.eye(4)
            origin[:3, 3] = t
            for i in rel:
                placed[i] = origin @ rel[i]
            self.log.append(f"{', '.join(self.occ[i].component for i in rel)}: contacts {support}")
            return True

        while len(placed) < len(self.occ):
            if by_joints(0.6) or by_contacts(50) or by_joints(0.4) or by_contacts(3) or by_joints(0.0):
                continue
            break
        checks = []
        for name, a, b, fa, fb, E in joints:
            fa2, fb2, ha, hb, _ = moved[name]
            if fa2 is None or fb2 is None or a not in placed or b not in placed:
                checks.append((name, None, ha, hb))
            else:
                checks.append((name, float(np.abs(placed[a] @ fa2 @ inverse(fb2) @ E - placed[b]).max()), ha, hb))
        return placed, checks


# -- the manifest ------------------------------------------------------------------------


def matrix_of(entry: dict) -> np.ndarray:
    return np.array(entry["matrix"]).reshape(4, 4).T


def print_fit(rotation: np.ndarray, step_vertices: np.ndarray, stl_points: np.ndarray) -> tuple[np.ndarray, float]:
    """The translation that lands the rotated STEP vertices on the STL's vertices, and the share landed."""
    keys = {tuple(v) for v in np.round(stl_points, 2)}
    moved = step_vertices @ rotation.T
    targets = np.unique(stl_points.round(3), axis=0)
    best = (np.zeros(3), -1)
    for direction in np.vstack([np.eye(3), -np.eye(3)]):
        i = int(np.argmax(moved @ direction))
        for s in targets[np.argsort(-(targets @ direction))[:300]]:
            t = s - moved[i]
            n = sum(tuple(v) in keys for v in np.round(moved + t, 2))
            if n > best[1]:
                best = (t, n)
        if best[1] == len(moved):
            break
    return best[0], best[1] / len(moved)


def stl_shift(a: Path, b: Path) -> np.ndarray | None:
    """The translation from one STL's triangles to another's, if they are the same triangles moved."""
    A, B = read_stl(a), read_stl(b)
    if A.shape == B.shape:
        d = B - A
        if np.abs(d - d[0]).max() < 1e-3:
            return d[0]
    # Tessellated again (a round part): the same extent counts as the same pose.
    size_a, size_b = A.max(0) - A.min(0), B.max(0) - B.min(0)
    centre = (B.max(0) + B.min(0) - A.max(0) - A.min(0)) / 2
    if np.abs(size_a - size_b).max() < 0.05 and np.abs(centre).max() < 0.05:
        return np.zeros(3)
    return None


def build(preset: str, base: Path, out: Path, f3d: Path = DEFAULT_F3D) -> dict:
    """The manifest at `preset`, from the SG90 manifest in `base` (extract.py's output)."""
    stock = json.loads((base / "assembly.json").read_text())
    solver = Solver(f3d, stock["preset"], preset)
    placed, checks = solver.solve()
    problems = [f"{solver.occ[i].component}: not placed" for i in solver.occ if i not in placed]
    by_component: dict[str, list[int]] = {}
    for inst, o in solver.occ.items():
        by_component.setdefault(o.component, []).append(inst)
    stl_dir = PARTS / preset / "stl"
    parts = []
    for entry in stock["parts"]:
        component = entry["component"]
        inst = by_component[component][0]
        occurrence = solver.occ[inst].matrix
        to_print = inverse(matrix_of(entry)) @ np.array(entry["occurrence"]).reshape(4, 4).T
        part = solver.part(component)
        stl = stl_dir / f"{component.replace('(Mirror)', '')}.stl"
        if part is None or not stl.exists():
            problems.append(f"{component}: no {preset} STEP or STL")
            continue
        delta = solver.delta(component)
        rotation = to_print[:3, :3]
        # The SG90 print transform from the STEP frame (design frame + delta).
        stock_step_to_print = to_print.copy()
        stock_step_to_print[:3, 3] = to_print[:3, 3] - rotation @ delta
        stock_stl = ROOT / entry["mesh"]
        step_to_print = np.eye(4)
        moved_print = stl_shift(stock_stl, stl) if part.shift is not None else None
        if moved_print is not None:
            # Same shape: the part only moved in its frame, and its print file at most moved too.
            step_to_print = stock_step_to_print.copy()
            step_to_print[:3, 3] += moved_print - rotation @ part.shift
            share = 1.0
        else:
            V = np.array([q for k, q in part.dst if k == "v"])
            t, share = print_fit(rotation, V, read_stl(stl))
            if share < MIN_PRINT_FIT:
                problems.append(f"{component}: only {share:.0%} of its {preset} STEP vertices land on {stl.name}")
            step_to_print[:3, :3], step_to_print[:3, 3] = rotation, t
        shift = np.eye(4)
        shift[:3, 3] = -delta
        matrix = placed[inst] @ shift @ inverse(step_to_print)
        parts.append({
            **{k: v for k, v in entry.items() if k not in ("matrix", "occurrence", "mesh", "method", "matched_vertices", "box_error_mm", "rest")},
            "mesh": stl.resolve().relative_to(ROOT).as_posix(),
            "matrix": column_major(matrix),
            "occurrence": column_major(placed[inst] @ shift),
            "method": "preset",
            "print_fit": round(share, 3),
        })
    # Hardware: each servo, the OLED and the board by its occurrence; the servo's mesh at this preset.
    (out / "hardware").mkdir(parents=True, exist_ok=True)
    vertices, triangles = servo_mesh(servo_params(preset))
    write_binary_stl(out / "hardware" / "Servo.stl", vertices, triangles)
    hardware = []
    servos = by_component.get("Servo", [])
    for entry in stock["hardware"]:
        # The ESP32 is seated again below; screws and nuts are placed by fasteners.py at the preset.
        if entry["id"] in ("ESP32-C3-Zero", "ESP32-pin-headers") or entry.get("source") == "fastener":
            continue
        if entry["component"] == "Servo":
            m = matrix_of(entry)
            inst = min(servos, key=lambda i: np.abs(solver.occ[i].matrix - m).max())
            mesh = "hardware/Servo.stl"
        else:
            inst = by_component[entry["component"]][0]
            mesh = entry["mesh"]
            shutil.copyfile(base / mesh, out / mesh)
        # The KiCad board keeps its offset from the design's board (extract.py seat_electronics).
        offset = np.array(entry["design_offset"]).reshape(4, 4).T if "design_offset" in entry else np.eye(4)
        hardware.append({**entry, "mesh": mesh, "matrix": column_major(placed[inst] @ offset), "source": "preset"})
        if entry["id"] == "MainControlBoard":
            frame = placed[inst]
    board = next((h for h in hardware if h["id"] == "MainControlBoard"), None)
    if board is not None:
        seat = np.array(board["esp32_seat"]).reshape(4, 4).T
        hardware += electronics(frame, np.array(board["design_offset"]).reshape(4, 4).T, seat, out, "canfactory")
    lower_arms(parts, hardware)
    if problems:
        raise SystemExit(f"{preset}: assembly failed:\n  " + "\n  ".join(problems))
    overridden = [{"name": name, "mismatch_mm": round(err, 3)} for name, err, _, _ in checks if err is not None and err > 0.01]
    return {
        "format": FORMAT,
        "preset": preset,
        "source": stock["source"],
        "units": "mm",
        "up": "+z",
        "derived_from": stock["preset"],
        "parts": parts,
        "hardware": hardware,
        "solve": solver.log,
        "joints_overridden": overridden,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--preset", required=True, help="servo preset id (servos.json servo_id)")
    parser.add_argument("--base", type=Path, required=True, help="extract.py's output for the saved preset")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--f3d", type=Path, default=DEFAULT_F3D)
    args = parser.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = build(args.preset, args.base, args.out, args.f3d)
    (args.out / "assembly.json").write_text(json.dumps(manifest, indent=1) + "\n")
    print(f"{len(manifest['parts'])} parts, {len(manifest['hardware'])} hardware at {args.preset}; "
          f"{len(manifest['joints_overridden'])} joints overridden -> {args.out / 'assembly.json'}")
    for line in manifest["solve"]:
        print(f"  {line}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
