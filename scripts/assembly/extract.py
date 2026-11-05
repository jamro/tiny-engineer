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

A mod's design (``--mod``) yields only the parts the mod exports, in its own
frame.
"""

from __future__ import annotations

import argparse
import itertools
import json
import shutil
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from boards import ESP32_PIN_CENTRE, ESP32_ROW_SPACING, HEADER_BODY  # noqa: E402
from meshes import write_stl  # noqa: E402
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
#: The design is saved with the hands raised about 30° above the shoulders. The
#: guide mounts each arm so that, with the servos at 90°, the hand sits at
#: shoulder height and the forearm points forward (docs/3d/assembly.md, step
#: 18), and the viewer's poses start from there. Each arm is turned this many
#: degrees down about its servo's shaft (the servo frame's +Y).
ARM_REST_DOWN_DEG = 29.0
ARM_PARTS = {
    "servo-arm-left": ("UpperArm", "ForearmLeft"),
    "servo-arm-right": ("UpperArmMirror", "ForearmRight"),
}


def column_major(matrix: np.ndarray) -> list[float]:
    """A 4x4 as glTF writes it: 16 numbers, column by column, rounded to 1 nm."""
    return [round(float(value), 6) + 0.0 for value in matrix.T.reshape(-1)]


def matrix_of(entry: dict, key: str = "matrix") -> np.ndarray:
    """A manifest entry's 4x4 (column_major's 16 numbers, under `key`)."""
    return np.array(entry[key], dtype=float).reshape(4, 4).T


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


#: Electronics the design does not have, drawn by boards.py: the ESP32-C3-Zero
#: (component side up, PCB corner at the origin, pins in two columns along +Y,
#: USB-C toward +Y), its pin headers in its frame, and the main control board
#: from KiCad in the frame of the design's simplified board.
HARDWARE_DIR = ROOT / "scripts" / "assembly" / "hardware"
ESP32_MESH = HARDWARE_DIR / "ESP32-C3-Zero.stl"
HEADERS_MESH = HARDWARE_DIR / "ESP32-C3-Zero-headers.stl"
BOARD_MESH = HARDWARE_DIR / "MainControlBoard.stl"
ESP32_PIN_ROW_LENGTH = 8 * 2.54
#: Largest mismatch (mm) between the module's pin rows and the sockets.
MAX_SEAT_MM = 0.1


def esp32_seat(board_vertices: np.ndarray) -> np.ndarray:
    """Where the ESP32-C3-Zero sits in MainControlBoard's frame (4x4).

    The board is modelled with the two 9-pin sockets (J7, J8) and the
    module's header spacer on its back, which faces down in its own frame:
    the module sits on that stack's top face (the board mesh's lowest face).
    J7 carries 5 V, GND and 3.3 V on pins 1-3 (hardware/boards/
    main-control-board/expected-nets.yml), so the module's 5 V column goes in
    the socket at -Y of the board frame and its USB-C end toward +X: that
    puts its RESET button under LampButton and its LED under the lamp, as
    the guide has it (docs/3d/assembly.md, section 13).
    """
    floor = board_vertices[:, 2].min()
    top = board_vertices[np.abs(board_vertices[:, 2] - floor) < 1e-3]
    ys = np.unique(top[:, 1].round(3))
    if len(ys) != 4:
        raise SystemExit(f"MainControlBoard: expected two socket rows on the stack's top face, found y {ys}")
    rows = np.array([(ys[0] + ys[1]) / 2, (ys[2] + ys[3]) / 2])
    centre_x = (top[:, 0].min() + top[:, 0].max()) / 2
    if abs(rows[1] - rows[0] - ESP32_ROW_SPACING) > MAX_SEAT_MM or np.ptp(top[:, 0]) < ESP32_PIN_ROW_LENGTH:
        raise SystemExit(f"MainControlBoard: sockets at y {rows.round(3)} do not take the ESP32-C3-Zero's pins")
    # Module X (across the pins) along the board's +Y, module Y (toward USB-C)
    # along its +X, component side away from the board (its -Z).
    seat = np.eye(4)
    seat[:3, :3] = np.array([[0, 1, 0], [1, 0, 0], [0, 0, -1]], dtype=float)
    seat[:3, 3] = np.array([centre_x, rows.mean(), floor]) - seat[:3, :3] @ np.array([*ESP32_PIN_CENTRE, 0.0])
    return seat




def seat_electronics(design_vertices: np.ndarray, board_vertices: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """The KiCad board's offset in the design board's frame, and the module's seat (4x4 each).

    The module stays where the design puts it (on the design's socket stack, which
    leaves it on the desk's ledge under LampBase). The KiCad board hangs from it:
    its sockets' tops (its lowest face) a header body below the module. Its socket
    rows must take the module's pins, to MAX_SEAT_MM.
    """
    seat = esp32_seat(design_vertices)
    floor = design_vertices[:, 2].min()
    low = board_vertices[:, 2].min()
    top = board_vertices[np.abs(board_vertices[:, 2] - low) < 1e-3]
    ys = np.sort(top[:, 1])
    split = int(np.argmax(np.diff(ys))) + 1
    rows = np.array([(ys[0] + ys[split - 1]) / 2, (ys[split] + ys[-1]) / 2])
    centre_x = (top[:, 0].min() + top[:, 0].max()) / 2
    pins = [seat @ np.array([x, ESP32_PIN_CENTRE[1], 0.0, 1.0]) for x in (ESP32_PIN_CENTRE[0] - ESP32_ROW_SPACING / 2, ESP32_PIN_CENTRE[0] + ESP32_ROW_SPACING / 2)]
    want = np.sort([pin[1] for pin in pins])
    if np.abs(rows - want).max() > MAX_SEAT_MM or abs(centre_x - pins[0][0]) > MAX_SEAT_MM:
        raise SystemExit(f"MainControlBoard: the KiCad board's sockets (rows y {rows.round(3)}, centre x {centre_x:.3f}) "
                         f"do not line up with the design's (y {want.round(3)}, x {pins[0][0]:.3f})")
    offset = np.eye(4)
    offset[2, 3] = floor + HEADER_BODY - low
    return offset, seat


def electronics(frame: np.ndarray, seat: np.ndarray, out: Path) -> list[dict]:
    """The module and its headers on the board at `frame` (the design board's frame)."""
    for mesh in (ESP32_MESH, HEADERS_MESH):
        shutil.copyfile(mesh, out / "hardware" / mesh.name)
    return [
        {"id": "ESP32-C3-Zero", "component": "ESP32-C3-Zero", "mesh": f"hardware/{ESP32_MESH.name}",
         "matrix": column_major(frame @ seat), "source": "drawing"},
        {"id": "ESP32-pin-headers", "component": "PinHeader", "mesh": f"hardware/{HEADERS_MESH.name}",
         "matrix": column_major(frame @ seat), "source": "drawing"},
    ]


def rotation_about(origin: np.ndarray, axis: np.ndarray, degrees: float) -> np.ndarray:
    """4x4 rotation by ``degrees`` (right hand) about the line through ``origin`` along ``axis``."""
    x, y, z = axis / np.linalg.norm(axis)
    c, s = np.cos(np.radians(degrees)), np.sin(np.radians(degrees))
    k = np.array([[0, -z, y], [z, 0, -x], [-y, x, 0]])
    result = np.eye(4)
    result[:3, :3] = np.eye(3) + s * k + (1 - c) * k @ k
    result[:3, 3] = origin - result[:3, :3] @ origin
    return result


def lower_arms(parts: list[dict], hardware: list[dict], degrees: float = ARM_REST_DOWN_DEG) -> None:
    """Turn each arm down about its servo shaft, to the pose the guide mounts it in."""
    servos = {h["id"]: matrix_of(h) for h in hardware}
    by_id = {part["id"]: part for part in parts}
    for servo, ids in ARM_PARTS.items():
        if servo not in servos:
            continue
        frame = servos[servo]
        axis = frame[:3, 1]
        # Down is about +y of the robot (+x, its front, turns toward -z).
        sign = 1.0 if axis[1] > 0 else -1.0
        turn = rotation_about(frame[:3, 3], axis, sign * degrees)
        for part_id in ids:
            part = by_id.get(part_id)
            if part is None:
                continue
            for key in ("matrix", "occurrence"):
                matrix = matrix_of(part, key)
                part[key] = column_major(turn @ matrix)
            part["rest"] = f"turned {degrees:g} deg down about {servo}"


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


def extract(f3d: Path, preset: str, parts_dir: Path, out: Path, repo_root: Path, *, mod: bool = False) -> dict:
    """The manifest for one design. With ``mod``, the design is a mod's: only
    the parts the mod exports are taken (the others come from the stock robot)
    and hardware is left out."""
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
    seen: set[str] = set()
    for occurrence in sorted(assembled, key=lambda o: (o.path, o.source != "record")):
        name = occurrence.component
        if name in NOT_IN_ROBOT or name == "PRINT_LAYOUT" or name.startswith("PartsSet"):
            continue
        # A component placed once (hardware is placed per occurrence).
        if name not in HARDWARE:
            if name in seen:
                continue
            seen.add(name)
        if name in HARDWARE and mod:
            continue
        if name in HARDWARE:
            if name not in hardware_meshes:
                vertices, triangles = hardware_mesh(design, name)
                path = out / "hardware" / f"{name}.stl"
                write_stl(path, vertices[triangles], header="Tiny Engineer hardware mesh")
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
            if not mod:
                problems.append(f"{name}: no {stl.resolve().relative_to(repo_root)}")
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
                "mesh": stl.resolve().relative_to(repo_root).as_posix(),
                # print pose (STL) -> component frame -> assembled robot
                "matrix": column_major(occurrence.matrix @ inverse(to_print)),
                # component frame -> assembled robot (to align a mod's parts)
                "occurrence": column_major(occurrence.matrix),
                "method": method,
                **(check or {"matched_vertices": [result.matched, result.total]}),
            }
        )

    # Joints are checked against solved transforms only, not assumed ones.
    placed = {o.instances[-1]: o.matrix for o in occurrences if o.source != "identity"}
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
    if unnamed and not mod:
        problems.append(f"{len(unnamed)} servo(s) not seated by any of the joints {sorted(SERVO_JOINTS)}")

    if not mod:
        lower_arms(parts, hardware)
        board = next((h for h in hardware if h["id"] == "MainControlBoard"), None)
        if board is None:
            problems.append("no MainControlBoard to seat the ESP32-C3-Zero on")
        else:
            # The design's board is a simplified one: seat the module on it, then put the
            # KiCad board in its place, hanging from the module's headers.
            design_vertices = np.asarray(read_stl(out / board["mesh"]), dtype=float).reshape(-1, 3)
            frame = matrix_of(board)
            offset, seat = seat_electronics(design_vertices, np.asarray(read_stl(BOARD_MESH), dtype=float).reshape(-1, 3))
            shutil.copyfile(BOARD_MESH, out / board["mesh"])
            board.update({"matrix": column_major(frame @ offset), "source": "kicad",
                          "design_offset": column_major(offset), "esp32_seat": column_major(seat)})
            hardware += electronics(frame, seat, out)

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
    parser.add_argument("--mod", action="store_true", help="a mod's design: only its exported parts, no hardware")
    args = parser.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    manifest = extract(args.f3d, args.preset, args.parts, args.out, ROOT, mod=args.mod)
    (args.out / "assembly.json").write_text(json.dumps(manifest, indent=1) + "\n")
    print(
        f"{len(manifest['parts'])} parts, {len(manifest['hardware'])} hardware, "
        f"{len(manifest['joints_checked'])} joints checked -> {args.out / 'assembly.json'}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
