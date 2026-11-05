"""Closed, outward-wound triangle meshes for the assembly model's generated parts
(boards.py, fasteners.py), and binary STL output.

A Mesh is triangles with a colour each (sRGB 0..1, or None). write_stl()
stores the colours in the attribute bytes, VisCAM style (bit 15, then 5 bits
each of red, green, blue), as build_glb.mjs reads them. Numpy only.
"""

from __future__ import annotations

import math
import struct
from pathlib import Path

import numpy as np


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
        """Write the mesh as a binary STL with its colours; `transform` maps (n, 3) points."""
        corners = self.corners()
        if transform is not None:
            corners = transform(corners.reshape(-1, 3)).reshape(-1, 3, 3)
        return write_stl(path, corners, self.colors, header)

    def volume(self, origin=(0.0, 0.0, 0.0)) -> float:
        """Signed volume, from `origin`: positive for outward faces, and the same from any
        origin only when the mesh is closed."""
        c = self.corners() - np.asarray(origin, float)
        return float((np.cross(c[:, 0], c[:, 1]) * c[:, 2]).sum() / 6)

    def closed(self) -> bool:
        return abs(self.volume() - self.volume((7.0, -11.0, 5.0))) < 1e-6 * max(1.0, abs(self.volume()))

    @classmethod
    def indexed(cls, vertices, triangles, color=None) -> "Mesh":
        mesh = cls()
        mesh.add(np.asarray(vertices, float)[np.asarray(triangles)], color)
        return mesh


def write_stl(path: Path, corners: np.ndarray, colors=None, header: str = "") -> int:
    """A binary STL of triangle corners (n, 3, 3); with a colour per triangle (sRGB, or None)
    in its attribute bytes. Returns the triangle count."""
    corners = np.asarray(corners, float).reshape(-1, 3, 3)
    normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
    lengths = np.linalg.norm(normals, axis=1, keepdims=True)
    normals = np.divide(normals, lengths, out=np.zeros_like(normals), where=lengths > 0)
    record = np.dtype([("normal", "<f4", 3), ("vertices", "<f4", (3, 3)), ("attr", "<u2")])
    data = np.zeros(len(corners), dtype=record)
    data["normal"], data["vertices"] = normals, corners
    if colors is not None and any(c is not None for c in colors):
        five = lambda c: min(31, max(0, round(c * 31)))  # noqa: E731
        data["attr"] = [0x8000 | (five(c[0]) << 10) | (five(c[1]) << 5) | five(c[2]) if c is not None else 0 for c in colors]
    path = Path(path)
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


def revolve(profile: list[tuple[float, float]], segments: int = 24, color=None) -> Mesh:
    """A closed solid of revolution about +z from (radius, z) points, axis to axis."""
    assert profile[0][0] == 0 and profile[-1][0] == 0
    angles = np.arange(segments) * 2 * math.pi / segments
    vertices = [np.array([0.0, 0.0, profile[0][1]])]
    rings = []
    for radius, z in profile[1:-1]:
        rings.append(len(vertices))
        vertices.extend(np.array([radius * math.cos(a), radius * math.sin(a), z]) for a in angles)
    vertices.append(np.array([0.0, 0.0, profile[-1][1]]))
    top = len(vertices) - 1
    triangles = []
    for i in range(segments):
        j = (i + 1) % segments
        triangles.append((0, rings[0] + j, rings[0] + i))
        for lower, upper in zip(rings, rings[1:]):
            triangles += [(lower + i, lower + j, upper + j), (lower + i, upper + j, upper + i)]
        triangles.append((top, rings[-1] + i, rings[-1] + j))
    return Mesh.indexed(np.array(vertices), np.array(triangles), color)


def along_y(mesh: Mesh) -> Mesh:
    """A mesh built along z (prism, box) turned to run along +y: (x, y, z) -> (x, z, -y), a rotation."""
    out = Mesh()
    for corners, color in zip(mesh.corners(), mesh.colors):
        out.add(np.column_stack([corners[:, 0], corners[:, 2], -corners[:, 1]]), color)
    return out
