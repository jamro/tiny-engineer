#!/usr/bin/env python3
"""The M2 screws and nuts of the assembled robot, for the 3D viewer.

Adds a hardware entry per screw and nut of ``fasteners.json`` to an
assembly.json (extract.py's or presets.py's), with its mesh beside it, and
fails when a line of the list does not find as many screws as it says.

Where a screw goes comes from the parts' STEP exports at the manifest's
preset (``3d_models/parts/<preset>/step/``), placed where the manifest has
each part:

- every circle of a hole (the pilot radius, ``screw_thread_diameter`` / 2,
  and the extra radii a line names) lies on an axis; circles of several parts
  on one axis are one hole stack, each part's circles in pairs giving the
  stretch of the axis its hole spans;
- a screw head sits at an end of one of the head part's holes with nothing
  of the stack just outside it (a counterbore floor, or the stack's end), and
  its shaft runs inward. It is that line's screw when the shaft, at its
  length, reaches into the part the line names: a stretch of that part's
  hole, or, for hardware (a servo's mounting tab), its mesh;
- when the head sits on hardware (the neck servo's tab), the head is where a
  ring of rays from the end of the part's hole leaves the hardware's mesh;
- a nut sits in a hex pocket of the part the screw threads into (``pocket``:
  that part's vertices around the axis, on the pocket floor), or against the
  far side of that part (``far``: the bell's nut under the desk).

    python3 scripts/assembly/fasteners.py artifacts/assembly/sg90/assembly.json
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from extract import ROOT, column_major, write_binary_stl  # noqa: E402
from f3d_assembly import inverse, read_stl  # noqa: E402
from presets import MIRRORED, matrix_of, print_fit, step_features  # noqa: E402

CONFIG = Path(__file__).resolve().parent / "fasteners.json"
STEPS = ROOT / "web" / "assembly" / "steps.json"
#: Circles this close to one axis (mm) are on one hole stack (the desk's holes sit 0.2 mm off the chair's).
AXIS_MM = 0.25
#: A screw must reach this far (mm) into the part it threads into.
MIN_BITE_MM = 1.0
#: Rays around a hole's axis, at this radius (mm): outside a servo tab's hole, under a screw head.
RING_MM = 1.7
RING_RAYS = 8
#: Hex pocket vertices sit this far from the axis (mm; an M2 nut's corners are 2.31 mm out).
POCKET_MM = (2.1, 2.7)
#: How far a screw starts out of its hole in its build step, past its length (mm); a nut, in all.
SCREW_SLIDE_MM = 3.0
#: check_clipping.mjs's default tolerance (mm).
CLIP_TOLERANCE_MM = 0.3
NUT_SLIDE_MM = 6.0


# -- meshes ------------------------------------------------------------------------------


def revolve(profile: list[tuple[float, float]], segments: int = 24) -> tuple[np.ndarray, np.ndarray]:
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
    return np.array(vertices), np.array(triangles)


def screw_mesh(length: float, head_diameter: float, head_height: float, thread_diameter: float) -> tuple[np.ndarray, np.ndarray]:
    """A pan-head screw: head on z 0..head_height, shaft down to z = -length with a pointed tip."""
    r, R, k = thread_diameter / 2, head_diameter / 2, head_height
    tip = min(0.6, length / 4)
    return revolve([
        (0.0, -length), (0.35 * r, -length), (r, -length + tip), (r, 0.0), (R, 0.0),
        (R, 0.55 * k), (0.9 * R, 0.85 * k), (0.65 * R, k), (0.0, k),
    ])


def nut_mesh(across_flats: float, height: float, hole_diameter: float, per_side: int = 4) -> tuple[np.ndarray, np.ndarray]:
    """A hex nut on z 0..height, corners on the x axis and every 60°."""
    corner = across_flats / math.sqrt(3)
    outer = []
    for side in range(6):
        a, b = side * math.pi / 3, (side + 1) * math.pi / 3
        p, q = corner * np.array([math.cos(a), math.sin(a)]), corner * np.array([math.cos(b), math.sin(b)])
        outer += [p + (q - p) * t / per_side for t in range(per_side)]
    n = len(outer)
    inner = [hole_diameter / 2 * np.array([math.cos(2 * math.pi * i / n), math.sin(2 * math.pi * i / n)]) for i in range(n)]
    vertices = np.array([[*xy, z] for z in (0.0, height) for xy in (*outer, *inner)])
    ob, ib, ot, it = 0, n, 2 * n, 3 * n
    triangles = []
    for i in range(n):
        j = (i + 1) % n
        triangles += [(ot + i, ot + j, it + j), (ot + i, it + j, it + i)]  # top
        triangles += [(ob + i, ib + j, ob + j), (ob + i, ib + i, ib + j)]  # bottom
        triangles += [(ob + i, ob + j, ot + j), (ob + i, ot + j, ot + i)]  # flats
        triangles += [(ib + i, it + j, ib + j), (ib + i, it + i, it + j)]  # hole
    return vertices, np.array(triangles)


# -- geometry ----------------------------------------------------------------------------


def perpendicular(d: np.ndarray) -> np.ndarray:
    e = np.cross(d, [0.0, 0.0, 1.0] if abs(d[2]) < 0.9 else [1.0, 0.0, 0.0])
    return e / np.linalg.norm(e)


def ray_hits(triangles: np.ndarray, origin: np.ndarray, direction: np.ndarray) -> np.ndarray:
    """Distances along a ray to every triangle it crosses, sorted (Möller–Trumbore)."""
    a, b, c = triangles[:, 0], triangles[:, 1], triangles[:, 2]
    e1, e2 = b - a, c - a
    p = np.cross(direction, e2)
    det = (e1 * p).sum(1)
    ok = np.abs(det) > 1e-12
    inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
    s = origin - a
    u = (s * p).sum(1) * inv
    q = np.cross(s, e1)
    v = (q @ direction) * inv
    t = (e2 * q).sum(1) * inv
    hit = ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 1e-9)
    # A ray through an edge shared by two triangles crosses the surface once.
    t = np.sort(t[hit])
    return t[np.concatenate([[True], np.diff(t) > 1e-6])] if len(t) else t


def ring(centre: np.ndarray, d: np.ndarray, radius: float = RING_MM, count: int = RING_RAYS) -> list[np.ndarray]:
    e1 = perpendicular(d)
    e2 = np.cross(d, e1)
    return [centre + radius * (math.cos(a) * e1 + math.sin(a) * e2) for a in np.arange(count) * 2 * math.pi / count]


def head_clear(triangles: np.ndarray, seat: np.ndarray, outward: np.ndarray, height: float = 1.4) -> bool:
    """Whether a screw head on `seat` is in the open: rays under the head, outward, meet none of
    the mesh within `height` (with ``height=inf``, none at all: the head is outside the part, not
    in a pocket inside it)."""
    # An odd count of crossings starts inside the mesh.
    return all(len(hits) % 2 == 0 and (not len(hits) or hits[0] > height)
               for hits in (ray_hits(triangles, origin, outward) for origin in ring(seat + 0.01 * outward, outward, radius=1.5)))


# -- the robot's holes -------------------------------------------------------------------


class Robot:
    """The manifest's parts with their STEP holes (world frame) and its hardware meshes."""

    def __init__(self, manifest: dict, here: Path, radii: set[float]) -> None:
        self.preset = manifest["preset"]
        self.entries = {entry["id"]: entry for entry in manifest["parts"]}
        self.hardware = {entry["id"]: entry for entry in manifest["hardware"] if entry.get("source") != "fastener"}
        self.here = here
        self.circles: list[tuple[str, float, np.ndarray, np.ndarray]] = []
        self.vertices: dict[str, np.ndarray] = {}
        fits: dict[str, tuple[np.ndarray, np.ndarray, float]] = {}
        steps = {}
        for entry in manifest["parts"]:
            name = MIRRORED.get(entry["component"], entry["component"])
            path = ROOT / "3d_models" / "parts" / self.preset / "step" / f"{name}.step"
            if not path.exists():
                continue
            steps[entry["id"]] = step_features(path)
            # STEP frame to the STL's print frame; a mesh used twice (the mirrored arm) takes the better fit.
            feats = steps[entry["id"]][0]
            V = np.array([q for k, q in feats if k == "v"])
            rotation = (inverse(matrix_of(entry)) @ np.array(entry["occurrence"]).reshape(4, 4).T)[:3, :3]
            shift, share = print_fit(rotation, V, read_stl(self.mesh_path(entry)))
            if share > fits.get(entry["mesh"], (None, None, -1))[2]:
                fits[entry["mesh"]] = (rotation, shift, share)
        for part, (feats, circles) in steps.items():
            entry = self.entries[part]
            rotation, shift, _ = fits[entry["mesh"]]
            M = matrix_of(entry)
            to_world = lambda x: M[:3, :3] @ (rotation @ x + shift) + M[:3, 3]  # noqa: E731
            self.vertices[part] = np.array([to_world(q) for k, q in feats if k == "v"])
            for radius, centre, axis in circles:
                if radius in radii:
                    d = M[:3, :3] @ rotation @ axis
                    self.circles.append((part, radius, to_world(centre), d / np.linalg.norm(d)))
        self.meshes: dict[str, np.ndarray] = {}

    def mesh_path(self, entry: dict) -> Path:
        mesh = Path(entry["mesh"])
        return mesh if mesh.is_absolute() else ROOT / mesh if entry["mesh"].startswith(("3d_models/", "mods/")) else self.here / mesh

    def triangles(self, part: str) -> np.ndarray:
        """A part's or hardware's mesh in the world frame, (n, 3, 3)."""
        if part not in self.meshes:
            entry = self.entries.get(part) or self.hardware[part]
            M = matrix_of(entry)
            corners = read_stl(self.mesh_path(entry))
            self.meshes[part] = (corners @ M[:3, :3].T + M[:3, 3]).reshape(-1, 3, 3)
        return self.meshes[part]

    def free(self, start: np.ndarray, direction: np.ndarray, limit: float, radius: float, skip: set[str]) -> float:
        """How far a disc of `radius` at `start` can travel along `direction` (up to `limit`) before
        it meets any mesh but those in `skip`."""
        ends = np.array([start, start + limit * direction])
        lo, hi = ends.min(0) - radius, ends.max(0) + radius
        best = limit
        for part in [*self.entries, *self.hardware]:
            if part in skip:
                continue
            tris = self.triangles(part)
            if (tris.reshape(-1, 3).min(0) > hi).any() or (tris.reshape(-1, 3).max(0) < lo).any():
                continue
            near = ((tris.min(1) <= hi) & (tris.max(1) >= lo)).all(1)
            if not near.any():
                continue
            for origin in [start, *ring(start, direction, radius), *ring(start, direction, radius / 2)]:
                hits = ray_hits(tris[near], origin, direction)
                if len(hits):
                    best = min(best, float(hits[0]))
        return best

    def lines(self) -> list[dict]:
        """Hole stacks: an axis (point, unit direction) and each part's (radius, offset along it) circles."""
        lines: list[dict] = []
        for part, radius, centre, d in self.circles:
            d = d if d[np.argmax(np.abs(d))] > 0 else -d
            for line in lines:
                if abs(abs(line["d"] @ d) - 1) < 1e-4:
                    off = centre - line["c"]
                    if np.linalg.norm(off - (off @ line["d"]) * line["d"]) < AXIS_MM:
                        line["circles"].append((part, radius, float(off @ line["d"]), centre))
                        break
            else:
                lines.append({"c": centre, "d": d, "circles": [(part, radius, 0.0, centre)]})
        return lines


def spans(line: dict, part: str, radii: set[float]) -> list[tuple[float, float]]:
    """The stretches of a line's axis that a part's holes of these radii span."""
    offsets = sorted({round(s, 3) for p, r, s, _ in line["circles"] if p == part and r in radii})
    return [(offsets[i], offsets[i + 1]) for i in range(0, len(offsets) - 1, 2)]


def inside(value: float, stretches: list[tuple[float, float]], margin: float = 1e-3) -> bool:
    return any(a + margin < value < b - margin for a, b in stretches)


def overlap(a: tuple[float, float], stretches: list[tuple[float, float]]) -> float:
    lo, hi = sorted(a)
    return sum(max(0.0, min(hi, b) - max(lo, s)) for s, b in stretches)


# -- placing -----------------------------------------------------------------------------


def frame(origin: np.ndarray, up: np.ndarray, angle: float = 0.0) -> np.ndarray:
    """A local frame at `origin` with +z along `up`, turned by `angle` about it."""
    e1 = perpendicular(up)
    e2 = np.cross(up, e1)
    x = math.cos(angle) * e1 + math.sin(angle) * e2
    M = np.eye(4)
    M[:3, 0], M[:3, 1], M[:3, 2], M[:3, 3] = x, np.cross(up, x), up, origin
    return M


def shown_by_step(steps: dict) -> dict[str, set[str]]:
    """Step title → the parts on the robot once that step is done (web/assembly/steps.json)."""
    shown: set[str] = set()
    result: dict[str, set[str]] = {}
    for section in steps["sections"]:
        for step in section["steps"]:
            shown |= set(step.get("add", []))
            result.setdefault(step["title"], set(shown))
    return result


def slide_mm(robot: Robot, seat: np.ndarray, outward: np.ndarray, spec: dict, config: dict, present: set[str] | None) -> float:
    """How far out of its hole a screw starts in its build step: clear of its hole by
    SCREW_SLIDE_MM, or as far as the space outside its head goes among the parts already
    there (the board under the desk)."""
    head = config["screw"]["head_height"]
    want = spec["length"] + SCREW_SLIDE_MM
    absent = {spec["head"]} | ({p for p in [*robot.entries, *robot.hardware] if p not in present} if present is not None else set())
    # Overlaps within the clipping check's tolerance are let through there, so here too.
    room = robot.free(seat + outward * (head + 0.05), outward, want, config["screw"]["head_diameter"] / 2 - CLIP_TOLERANCE_MM, absent) - 0.2
    return round(max(min(want, room), min(want, 2.0)), 3)


def place(manifest: dict, here: Path, config: dict, steps: dict | None = None) -> tuple[list[dict], list[str]]:
    """Fastener hardware entries for a manifest, and problems (lines that found the wrong count)."""
    shown = shown_by_step(steps) if steps else {}
    pilot = config["pilot_radius"]
    radii_of = lambda spec, part: {pilot, *spec.get("radii", {}).get(part, [])}  # noqa: E731
    every_radius = {pilot} | {r for spec in config["screws"] for rs in spec.get("radii", {}).values() for r in rs}
    robot = Robot(manifest, here, every_radius)
    lines = robot.lines()
    entries: list[dict] = []
    problems: list[str] = []
    taken: dict[tuple, str] = {}
    for spec in config["screws"]:
        head, into, length = spec["head"], spec["into"], float(spec["length"])
        found = []
        for index, line in enumerate(lines):
            d = line["d"]
            stack = [s for p in {p for p, *_ in line["circles"]} for s in spans(line, p, every_radius)]
            into_spans = spans(line, into, radii_of(spec, into)) if into in robot.entries else []
            # The axis through the head part's holes (or, for a head on hardware, the part's it goes into).
            own = [c for p, r, _, c in line["circles"] if p == (head if head in robot.entries else into)]
            if not own:
                continue
            base = np.mean([c - ((c - line["c"]) @ d) * d for c in own], axis=0)
            point = lambda s, base=base, d=d: base + s * d  # noqa: E731
            seats = []
            # What a head must clear: the parts of this stack, and the hardware the screw goes into.
            neighbours = {p for p, *_ in line["circles"]} | {head} | ({into} if into in robot.hardware else set())
            if head in robot.entries:
                for a, b in spans(line, head, radii_of(spec, head)):
                    for seat, sign in ((a, 1.0), (b, -1.0)):
                        if not inside(seat - sign * 0.3, stack, margin=0) and all(head_clear(robot.triangles(p), point(seat), -sign * d) for p in neighbours) \
                                and (spec.get("enclosed") or head_clear(robot.triangles(head), point(seat), -sign * d, math.inf)):
                            seats.append((seat, sign))
            elif into_spans:
                tris = robot.triangles(head)
                for a, b in into_spans:
                    for end, sign in ((a, -1.0), (b, 1.0)):  # sign: outward
                        exits = []
                        for origin in ring(point(end) - sign * 0.05 * d, d):
                            hits = ray_hits(tris, origin, sign * d)
                            # Starting inside the hardware (an odd count of crossings), the first crossing leaves it.
                            if len(hits) % 2:
                                exits.append(hits[0])
                            elif len(hits) >= 2 and hits[0] < 1.0:
                                exits.append(hits[1])
                        if len(exits) > RING_RAYS // 2:
                            seats.append((end + sign * (float(np.median(exits)) - 0.05), -sign))
            for seat, sign in seats:
                shaft = (seat, seat + sign * length)
                if into in robot.entries:
                    if overlap(shaft, into_spans) < spec.get("bite", MIN_BITE_MM):
                        continue
                else:
                    tris = robot.triangles(into)
                    crossing = sum(any(0 < t < length for t in ray_hits(tris, origin, sign * d)) for origin in ring(point(seat), d))
                    if crossing <= RING_RAYS // 2:
                        continue
                key = (index, round(seat, 2), sign)
                if key in taken:
                    problems.append(f"{spec['id']}: the screw at {np.round(point(seat), 2).tolist()} is also {taken[key]}'s")
                    continue
                taken[key] = spec["id"]
                found.append((line, point, seat, sign, into_spans))
        if len(found) != spec["count"]:
            where = [np.round(f[1](f[2]), 1).tolist() for f in found]
            problems.append(f"{spec['id']}: {len(found)} screws from {head} into {into}, the list says {spec['count']} {where}")
        found.sort(key=lambda f: tuple(np.round(f[1](f[2]), 2)))
        for n, (line, point, seat, sign, into_spans) in enumerate(found, 1):
            d = line["d"] * sign  # into the stack
            size = f"M2×{spec['length']:g}"
            entries.append({
                "id": f"screw-{spec['id']}-{n}", "component": f"M2x{spec['length']:g}", "mesh": f"hardware/M2x{spec['length']:g}.stl",
                "matrix": column_major(frame(point(seat), -d)), "source": "fastener",
                "fastener": {"kind": "screw", "size": size, "parent": head, "parts": [head, into], "step": spec["step"],
                             "from": np.round(-d * slide_mm(robot, point(seat), -d, spec, config, shown.get(spec["step"])), 3).tolist()},
            })
            nut = spec.get("nut")
            if not nut:
                continue
            height = config["nut"]["height"]
            holder = nut.get("in", into)
            if nut["at"] == "pocket":
                V = robot.vertices[holder]
                off = V - base
                along = off @ line["d"]
                radial = np.linalg.norm(off - np.outer(along, line["d"]), axis=1)
                lo, hi = sorted((seat, seat + sign * spec["length"]))
                near = (radial > POCKET_MM[0]) & (radial < POCKET_MM[1]) & (along > lo) & (along < hi)
                if near.sum() < 6:
                    problems.append(f"{spec['id']}: no hex pocket for the nut in {holder}")
                    continue
                # The floor is the pocket's closed end: the part goes on past it, inside the hex.
                ends = along[near].min(), along[near].max()
                tris = robot.triangles(holder)
                solid = [sum(len(ray_hits(tris, o, line["d"])) % 2 for o in ring(point(e + 0.1 * k), line["d"], radius=1.6)) > RING_RAYS // 2
                         for e, k in zip(ends, (-1, 1))]
                if solid[0] == solid[1]:
                    problems.append(f"{spec['id']}: the nut pocket in {holder} is open at {'both' if not solid[0] else 'neither'} end")
                    continue
                floor, opening = (ends[0], 1.0) if solid[0] else (ends[1], -1.0)
                up = line["d"] * opening
                origin = point(floor)
                e1 = perpendicular(up)
                corner = V[near][0] - point(along[near][0])
                angle = math.atan2(corner @ np.cross(up, e1), corner @ e1)
                slide = up * NUT_SLIDE_MM
            else:  # far: against the far side of the part it threads into
                far = max(b for a, b in into_spans) if sign > 0 else min(a for a, b in into_spans)
                origin, up, angle, slide = point(far) + d * height, -d, 0.0, d * NUT_SLIDE_MM
            entries.append({
                "id": f"nut-{spec['id']}-{n}", "component": "M2-nut", "mesh": "hardware/M2-nut.stl",
                "matrix": column_major(frame(origin, up, angle)), "source": "fastener",
                "fastener": {"kind": "nut", "size": "M2", "parent": holder, "parts": [holder], "step": nut["step"],
                             "from": np.round(slide, 3).tolist()},
            })
    return entries, problems


def add_fasteners(manifest_path: Path, config_path: Path = CONFIG, steps_path: Path = STEPS) -> tuple[dict, list[str]]:
    config = json.loads(config_path.read_text())
    manifest = json.loads(manifest_path.read_text())
    here = manifest_path.parent
    entries, problems = place(manifest, here, config, json.loads(steps_path.read_text()))
    screw, nut = config["screw"], config["nut"]
    for length in sorted({spec["length"] for spec in config["screws"]}):
        write_binary_stl(here / "hardware" / f"M2x{length:g}.stl", *screw_mesh(length, screw["head_diameter"], screw["head_height"], screw["thread_diameter"]))
    write_binary_stl(here / "hardware" / "M2-nut.stl", *nut_mesh(nut["width_across_flats"], nut["height"], screw["thread_diameter"]))
    manifest["hardware"] = [entry for entry in manifest["hardware"] if entry.get("source") != "fastener"] + entries
    manifest_path.write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest, problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--config", type=Path, default=CONFIG)
    parser.add_argument("--steps", type=Path, default=STEPS)
    args = parser.parse_args(argv)
    manifest, problems = add_fasteners(args.manifest, args.config, args.steps)
    fasteners = [entry for entry in manifest["hardware"] if entry.get("source") == "fastener"]
    if problems:
        print(f"{args.manifest}:\n  " + "\n  ".join(problems), file=sys.stderr)
        return 1
    screws = sum(entry["fastener"]["kind"] == "screw" for entry in fasteners)
    print(f"{screws} screws, {len(fasteners) - screws} nuts at {manifest['preset']} -> {args.manifest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
