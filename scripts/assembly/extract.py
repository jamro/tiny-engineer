#!/usr/bin/env python3
"""Write the assembled robot's layout from the Fusion design, for the 3D viewer.

Reads ``3d_models/cad/TinyEngineer.f3d`` (or a mod's design) and writes, into
``--out``:

- ``assembly.json``: for every printed part, the STL the Fusion add-in
  exported (print pose) and the transform that takes it to where it sits in
  the assembled robot; for every piece of hardware (servos, OLED, control
  board), its mesh and transform.
- ``hardware/<Component>.stl``: hardware meshes, tessellated from the design's
  own bodies (they have no exported STL).

Run with the servo preset the design was saved with: the transforms are
checked against that preset's STLs, and the run fails if any part does not
land on its exported mesh, or if a joint disagrees with the transforms.

    pip install -r scripts/assembly/requirements.txt   # Python 3.13
    python3 scripts/assembly/extract.py --preset sg90 --out artifacts/assembly/sg90
"""

from __future__ import annotations

import argparse
import itertools
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from f3d_assembly import (  # noqa: E402
    Design,
    Match,
    inverse,
    joint_agreement,
    match,
    read_stl,
    stl_targets,
)

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_F3D = ROOT / "3d_models" / "cad" / "TinyEngineer.f3d"
DEFAULT_PARTS = ROOT / "3d_models" / "parts"
FORMAT = 1

#: Printed calibration pieces: exported with the parts, not part of the robot.
NOT_IN_ROBOT = {"ServoSizingTester", "ScrewSizingTest"}
#: Components that are bought hardware rather than printed parts.
HARDWARE = {"Servo", "OLED", "MainControlBoard"}
#: Servos are named by the joint that seats them.
SERVO_JOINTS = {
    "Head Servo": "servo-head",
    "Neck - Body": "servo-neck",
    "UpperArm Left": "servo-arm-left",
    "UpperArm Right": "servo-arm-right",
    "ChairServo": "servo-body",
}
#: A part is placed when one of its solids lands on its STL with at least this
#: many vertices, and this share of them. Not all: a part exported before a
#: small parametric change (a screw pilot diameter) moves the vertices of its
#: holes, but not the part.
MIN_MATCHED_VERTICES = 16
MIN_MATCHED_FRACTION = 0.5
#: Exact coincidences this many times over cannot be chance, whatever the share.
CONCLUSIVE_MATCHED_VERTICES = 100
#: Largest acceptable mismatch (mm) between a named joint and the transforms.
MAX_JOINT_MM = 0.05


def column_major(matrix: np.ndarray) -> list[float]:
    """A 4x4 as glTF writes it: 16 numbers, column by column, rounded to 1 nm."""
    return [round(float(value), 6) + 0.0 for value in matrix.T.reshape(-1)]


def rotation_candidates(design: Design) -> list[np.ndarray]:
    """Rotations a part may be printed in relative to another: axis-aligned ones,
    and every rotation the design's occurrences use."""
    candidates: list[np.ndarray] = []

    def add(rotation: np.ndarray) -> None:
        if not any(np.abs(rotation - known).max() < 1e-4 for known in candidates):
            candidates.append(rotation)

    for permutation in itertools.permutations(range(3)):
        for signs in itertools.product((1, -1), repeat=3):
            rotation = np.zeros((3, 3))
            for row, column in enumerate(permutation):
                rotation[row, column] = signs[row]
            if np.linalg.det(rotation) > 0:
                add(rotation)
    for occurrence in design.occurrences():
        add(occurrence.matrix[:3, :3])
        add(occurrence.matrix[:3, :3].T)
    return candidates


def kabsch(source: np.ndarray, target: np.ndarray) -> np.ndarray:
    """Rigid transform that best maps ``source`` points onto ``target`` points."""
    source_centre, target_centre = source.mean(0), target.mean(0)
    u, _, vt = np.linalg.svd((source - source_centre).T @ (target - target_centre))
    d = np.diag([1, 1, np.sign(np.linalg.det(vt.T @ u.T))])
    rotation = vt.T @ d @ u.T
    result = np.eye(4)
    result[:3, :3] = rotation
    result[:3, 3] = target_centre - rotation @ source_centre
    return result


def accepted(result: Match) -> bool:
    return result.matched >= MIN_MATCHED_VERTICES and (
        result.fraction >= MIN_MATCHED_FRACTION or result.matched >= CONCLUSIVE_MATCHED_VERTICES
    )


def best_match(solids: list[np.ndarray], to_print: np.ndarray, targets: np.ndarray) -> Match:
    """The solid of a part that lands best on its STL (most vertices on it)."""
    results = [match(vertices, to_print, targets) for vertices in solids]
    return max(results, key=lambda result: (result.matched, result.fraction), default=Match(0, 0))


#: A small part also passes when this many of its few vertices, and this share,
#: coincide with the STL's.
MIN_SMALL_MATCHED = 6
MIN_SMALL_FRACTION = 0.85
#: For a part too round to have many vertices (a bell, a cap), its surface's
#: bounding box after the print transform must match the STL's within this (mm).
MAX_BOX_ERROR_MM = 0.3


def solid_surfaces(design: Design, blob: str) -> list[np.ndarray]:
    """Tessellated surface points (mm, component frame) of each solid in a blob."""
    from ezf3d.asm.brep import Shape
    from ezf3d.mesh.tessellate import tessellate_face

    result = []
    for solid in Shape(design.blobs[blob].model()).solids():
        points = []
        for face in solid.faces():
            # Coarser tolerances for curved faces the fine one cannot triangulate:
            # only the extent matters here.
            for tolerance in (0.002, 0.02, 0.1):
                mesh, reason, _ = tessellate_face(face, tolerance)
                if reason is None and mesh is not None and len(mesh.vertices):
                    points.append(mesh.vertices * 10.0)
                    break
        if points:
            result.append(np.vstack(points))
    return result


def box_error(surfaces: list[np.ndarray], to_print: np.ndarray, stl_points: np.ndarray) -> float:
    """Smallest bounding-box mismatch (mm) between a transformed solid and the STL."""
    low, high = stl_points.min(0), stl_points.max(0)
    best = float("inf")
    for points in surfaces:
        moved = points @ to_print[:3, :3].T + to_print[:3, 3]
        best = min(best, float(max(np.abs(moved.min(0) - low).max(), np.abs(moved.max(0) - high).max())))
    return best


def register(solids: list[np.ndarray], targets: np.ndarray, rotations: list[np.ndarray]) -> tuple[np.ndarray, Match]:
    """Fit a part onto its STL when the design has no usable print transform for
    it (a mirrored copy printed as its twin, a stale print record). Iterative
    closest point from each candidate rotation, on the part's larger solids."""
    best: tuple[np.ndarray, Match] | None = None
    for vertices in sorted(solids, key=len, reverse=True)[:3]:
        if len(vertices) < MIN_MATCHED_VERTICES:
            continue
        for rotation in rotations:
            transform = np.eye(4)
            transform[:3, :3] = rotation
            moved = vertices @ rotation.T
            transform[:3, 3] = (targets.min(0) + targets.max(0)) / 2 - (moved.min(0) + moved.max(0)) / 2
            for _ in range(30):
                moved = vertices @ transform[:3, :3].T + transform[:3, 3]
                squared = ((moved[:, None, :] - targets[None, :, :]) ** 2).sum(-1)
                index = squared.argmin(1)
                distances = np.sqrt(squared[np.arange(len(moved)), index])
                keep = distances <= np.percentile(distances, 90) + 1e-9
                transform = kabsch(vertices[keep], targets[index[keep]])
            result = best_match(solids, transform, targets)
            if best is None or (result.matched, result.fraction) > (best[1].matched, best[1].fraction):
                best = (transform, result)
            if result.fraction > 0.99:
                return best
    return best if best is not None else (np.eye(4), Match(0, 0))


def write_binary_stl(path: Path, vertices: np.ndarray, triangles: np.ndarray) -> None:
    corners = vertices[triangles]
    normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
    lengths = np.linalg.norm(normals, axis=1, keepdims=True)
    normals = np.divide(normals, lengths, out=np.zeros_like(normals), where=lengths > 0)
    record = np.dtype([("normal", "<f4", 3), ("vertices", "<f4", (3, 3)), ("attr", "<u2")])
    data = np.zeros(len(triangles), dtype=record)
    data["normal"] = normals
    data["vertices"] = corners
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(b"Tiny Engineer hardware mesh".ljust(80, b"\0"))
        handle.write(np.uint32(len(triangles)).tobytes())
        handle.write(data.tobytes())


def hardware_mesh(design: Design, component: str) -> tuple[np.ndarray, np.ndarray]:
    """Tessellate a hardware component's body (mm, component frame)."""
    from ezf3d.asm.brep import Shape
    from ezf3d.mesh.tessellate import tessellate

    best = None
    for blob in design.component(component).bodies:
        result = tessellate(Shape(design.blobs[blob].model()), measure=False)
        mesh = result.mesh
        if mesh.is_empty:
            continue
        # Prefer the body without stale history solids: fewest solids, then most faces.
        key = (result.solids, -result.faces_meshed)
        if best is None or key < best[0]:
            best = (key, mesh)
    if best is None:
        raise SystemExit(f"{component}: no body could be tessellated")
    mesh = best[1]
    return mesh.vertices * 10.0, mesh.triangles


def extract(f3d: Path, preset: str, parts_dir: Path, out: Path, repo_root: Path) -> dict:
    design = Design(f3d)
    occurrences = design.occurrences()
    stl_dir = parts_dir / preset / "stl"
    if not stl_dir.is_dir():
        raise SystemExit(f"{stl_dir}: no exported STLs for preset {preset!r}")

    assembled = [o for o in occurrences if not o.in_print_layout]
    print_pose = {o.component: o.matrix for o in occurrences if o.in_print_layout and len(o.path) == 2}
    # The aggregate a part is printed in (PartsSetA copper, PartsSetB black):
    # its colour as designed.
    print_group = {
        o.component: o.path[1]
        for o in occurrences
        if o.in_print_layout and len(o.path) == 3 and o.path[1].startswith("PartsSet")
    }
    rotations = None
    problems: list[str] = []
    parts = []
    hardware = []

    joints = design.joints()
    servo_names = {}
    for joint in joints:
        if joint.name in SERVO_JOINTS:
            for oid in joint.referenced:
                if design.instance_component.get(oid) == "Servo":
                    servo_names.setdefault(oid, SERVO_JOINTS[joint.name])

    hardware_meshes: dict[str, str] = {}
    for occurrence in sorted(assembled, key=lambda o: o.path):
        name = occurrence.component
        if name in NOT_IN_ROBOT:
            continue
        if name in HARDWARE:
            if name not in hardware_meshes:
                vertices, triangles = hardware_mesh(design, name)
                path = out / "hardware" / f"{name}.stl"
                write_binary_stl(path, vertices, triangles)
                hardware_meshes[name] = path.relative_to(out).as_posix()
            ident = servo_names.get(occurrence.instances[-1], name) if name == "Servo" else name
            hardware.append(
                {
                    "id": ident,
                    "component": name,
                    "mesh": hardware_meshes[name],
                    "matrix": column_major(occurrence.matrix),
                    "source": occurrence.source,
                }
            )
            continue

        component = design.component(name)
        stl_name = name.replace("(Mirror)", "")
        stl = stl_dir / f"{stl_name}.stl"
        if not stl.exists():
            problems.append(f"{name}: no {stl.relative_to(repo_root)}")
            continue
        stl_points = read_stl(stl)
        targets = stl_targets(stl_points)
        solids = [vertices for blob in component.bodies for vertices in design.solid_vertices(blob)]
        to_print, result, method = np.eye(4), Match(0, 0), "none"
        check = {}
        if name in print_pose and stl_name == name:
            to_print, method = print_pose[name], "print-layout"
            result = best_match(solids, to_print, targets)
            small = result.total < MIN_MATCHED_VERTICES and result.fraction == 1.0
            if not accepted(result) and (small or max((len(v) for v in solids), default=0) < MIN_MATCHED_VERTICES):
                # Round parts: check the surface's extent instead of its few vertices.
                surfaces = [p for blob in component.bodies for p in solid_surfaces(design, blob)]
                error = box_error(surfaces, to_print, stl_points)
                check = {"box_error_mm": round(error, 4)}
                if error <= MAX_BOX_ERROR_MM or (
                    result.matched >= MIN_SMALL_MATCHED and result.fraction >= MIN_SMALL_FRACTION
                ):
                    # The extent fits, or nearly every vertex coincides exactly
                    # (a dome the tessellator cannot triangulate shortens the box).
                    check["matched_vertices"] = [result.matched, result.total]
                    result = Match(MIN_MATCHED_VERTICES, MIN_MATCHED_VERTICES)
                else:
                    print(f"  {name}: surface lands {error:.3f} mm off {stl.name}", file=sys.stderr)
        if not accepted(result):
            if method == "print-layout":
                print(f"  {name}: print transform lands {result.matched}/{result.total} vertices; registering", file=sys.stderr)
            rotations = rotations or rotation_candidates(design)
            to_print, result = register(solids, targets, rotations)
            method = "registered"
        print(f"  {name:18s} {method:12s} {check or f'{result.matched}/{result.total} vertices'}", file=sys.stderr)
        if not accepted(result):
            problems.append(
                f"{name}: only {result.matched}/{result.total} vertices land on {stl.name} ({method})"
            )
        parts.append(
            {
                "id": name.replace("(Mirror)", "Mirror"),
                "component": name,
                "print_group": print_group.get(name),
                "mesh": stl.relative_to(repo_root).as_posix(),
                # print pose (STL) -> component frame -> assembled robot
                "matrix": column_major(occurrence.matrix @ inverse(to_print)),
                "method": method,
                **(check or {"matched_vertices": [result.matched, result.total]}),
            }
        )

    placed = {o.instances[-1]: o.matrix for o in occurrences}
    checked = []
    for joint in joints:
        if not joint.name or len(joint.name) == 36 and joint.name.count("-") == 4:
            continue
        mismatch = joint_agreement(joint, placed)
        if mismatch is None:
            continue
        checked.append({"name": joint.name, "mismatch_mm": round(mismatch, 5)})
        if mismatch > MAX_JOINT_MM:
            problems.append(f"joint {joint.name!r} disagrees with its occurrences by {mismatch:.3f} mm")

    servos = [h for h in hardware if h["component"] == "Servo"]
    unnamed = [h for h in servos if h["id"] == "Servo"]
    if unnamed:
        problems.append(f"{len(unnamed)} servo(s) not seated by any of the joints {sorted(SERVO_JOINTS)}")

    manifest = {
        "format": FORMAT,
        "preset": preset,
        "source": f3d.resolve().relative_to(repo_root).as_posix(),
        "units": "mm",
        "up": "+z",
        "parts": parts,
        "hardware": hardware,
        "joints_checked": checked,
    }
    if problems:
        raise SystemExit("assembly check failed:\n  " + "\n  ".join(problems))
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--f3d", type=Path, default=DEFAULT_F3D)
    parser.add_argument("--parts", type=Path, default=DEFAULT_PARTS, help="folder holding <preset>/stl/")
    parser.add_argument("--preset", default="sg90", help="servo preset the design was saved with")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = extract(args.f3d, args.preset, args.parts, args.out, ROOT)
    (args.out / "assembly.json").write_text(json.dumps(manifest, indent=1) + "\n")
    print(
        f"{len(manifest['parts'])} parts, {len(manifest['hardware'])} hardware, "
        f"{len(manifest['joints_checked'])} joints checked -> {args.out / 'assembly.json'}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
