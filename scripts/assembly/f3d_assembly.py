"""Read where every part of the robot sits from the Fusion design file.

The ``.f3d`` stores each component's bodies in that component's own frame, and
one record per occurrence with the occurrence's rigid transform (centimetres).
Fusion writes those records for every occurrence it has solved, including the
print copies under ``PRINT_LAYOUT``. This module finds them.

Nothing here is a documented format. It was read from the file with ezf3d's
container and object index (https://github.com/AlexSabaka/ezf3d, MIT), and
every result is checked against something independent before it is used:

- a printed part's print transform must map its B-Rep vertices onto the
  vertices of the STL the Fusion add-in exported (``print_residual``);
- named joints must agree with the transforms of the two occurrences they
  connect (``joint_agreement``).

``scripts/assembly/extract.py`` fails if either check fails.
"""

from __future__ import annotations

import os
import struct
from dataclasses import dataclass, field

import numpy as np

# ezf3d is imported where a design is read, so the matrix helpers here work
# (and are tested) with numpy alone.

CM_TO_MM = 10.0
#: Occurrence records are small; joint and feature records holding matrices are larger.
MAX_OCCURRENCE_RECORD = 400
#: Instance ids that stand for the root of the design, not an occurrence.
ROOT_IDS = {0}
PRINT_LAYOUT = "PRINT_LAYOUT"


def rigid_matrices(raw: bytes) -> list[tuple[int, np.ndarray]]:
    """Every rigid 4x4 transform stored as 16 little-endian doubles in ``raw``.

    Returns ``(byte offset, M)`` with the rotation in ``M[:3, :3]`` and the
    translation in ``M[:3, 3]``. Fusion writes the 16 values with the
    translation in the last row; the transposed layout is accepted too, so the
    reader does not depend on that.
    """
    found = []
    for align in range(8):
        count = (len(raw) - align) // 8
        if count < 16:
            continue
        values = np.frombuffer(raw[align : align + count * 8], dtype="<f8")
        for index in np.flatnonzero(values[15:] == 1.0):
            window = values[index : index + 16]
            if not np.all(np.isfinite(window)) or np.abs(window).max() > 1e5:
                continue
            matrix = window.reshape(4, 4)
            for candidate in (matrix.T, matrix):
                if np.any(candidate[3, :3] != 0):
                    continue
                rotation = candidate[:3, :3]
                if (
                    np.abs(rotation @ rotation.T - np.eye(3)).max() < 1e-6
                    and abs(np.linalg.det(rotation) - 1) < 1e-6
                ):
                    found.append((align + int(index) * 8, candidate.copy()))
                    break
    found.sort(key=lambda item: item[0])
    return found


def inverse(matrix: np.ndarray) -> np.ndarray:
    """Inverse of a rigid 4x4 transform."""
    result = np.eye(4)
    result[:3, :3] = matrix[:3, :3].T
    result[:3, 3] = -matrix[:3, :3].T @ matrix[:3, 3]
    return result


def to_mm(matrix: np.ndarray) -> np.ndarray:
    """A transform in centimetres (Fusion's kernel unit) as millimetres."""
    result = matrix.copy()
    result[:3, 3] *= CM_TO_MM
    return result


@dataclass
class Occurrence:
    """One placed instance of a component."""

    #: Component names from the outermost occurrence to this one.
    path: tuple[str, ...]
    #: Instance object ids along the same path.
    instances: tuple[int, ...]
    #: Transform from the component's frame to the design, millimetres.
    matrix: np.ndarray
    #: ``record``: Fusion's solved occurrence record; ``instance``: the
    #: placement stored on the instance itself, used when no record exists.
    source: str = "record"

    @property
    def component(self) -> str:
        return self.path[-1]

    @property
    def in_print_layout(self) -> bool:
        return PRINT_LAYOUT in self.path


@dataclass
class Joint:
    name: str
    #: The two instance ids it connects, as the record lists them.
    instances: tuple[int, int]
    #: Joint frames (component frames, centimetres), by byte offset in the record.
    frames: dict[int, np.ndarray] = field(default_factory=dict)
    #: Every instance id the record references (an occurrence path may be longer).
    referenced: tuple[int, ...] = ()


class Design:
    """The parts of a ``.f3d`` this pipeline needs."""

    def __init__(self, path: str | os.PathLike[str]) -> None:
        import ezf3d
        from ezf3d.model.design import read_design

        self.document = ezf3d.readfile(str(path))
        segment = self.document.design
        if segment is None:
            raise ValueError(f"{path}: no design segment")
        self.segment = segment
        self.body = segment.bulk.body
        self.items = segment.objects()
        self.by_id = {item.oid: item for item in self.items}
        self.known = set(self.by_id)
        self.graph = read_design(segment)
        self.components = {component.oid: component for component in self.graph.components}
        self.blobs = {os.path.basename(body.path): body for body in self.document.bodies}
        root = self.graph.roots.get("ComponentInstancesRoot")
        if root is None:
            raise ValueError(f"{path}: no ComponentInstancesRoot")
        self.instance_ids = [oid for oid in self.refs(root) if oid not in ROOT_IDS]
        self.instance_component = {}
        for oid in self.instance_ids:
            names = [self.components[r].name for r in self.refs(oid) if r in self.components]
            if names:
                self.instance_component[oid] = names[0]

    # -- raw object access -------------------------------------------------

    def raw(self, oid: int) -> bytes:
        item = self.by_id[oid]
        return self.body[item.offset : item.end]

    def refs(self, oid: int) -> list[int]:
        from ezf3d.model.design import references

        return references(self.body, self.by_id[oid], self.known)

    def strings(self, oid: int, min_len: int = 2) -> list[str]:
        from ezf3d.streams.primitives import scan_strings

        item = self.by_id[oid]
        return [s.value for s in scan_strings(self.body, start=item.offset, end=item.end, min_len=min_len)]

    # -- occurrences -------------------------------------------------------

    def occurrences(self) -> list[Occurrence]:
        """Every occurrence with a solved transform, outermost first in each path."""
        instances = set(self.instance_component)
        found: dict[tuple[int, ...], Occurrence] = {}
        for item in self.items:
            if item.oid in instances or item.size > MAX_OCCURRENCE_RECORD:
                continue
            path_ids = tuple(r for r in self.refs(item.oid) if r in instances)
            if not path_ids:
                continue
            matrices = rigid_matrices(self.body[item.offset : item.end])
            if len(matrices) != 1:
                continue
            path = tuple(self.instance_component[r] for r in path_ids)
            found.setdefault(path_ids, Occurrence(path, path_ids, to_mm(matrices[0][1])))
        # An instance Fusion wrote no record for keeps the placement stored on
        # the instance itself (e.g. the body servo, placed by joints only).
        placed = {ids[-1] for ids in found}
        for oid in self.instance_ids:
            if oid in placed or oid not in self.instance_component:
                continue
            matrices = rigid_matrices(self.raw(oid))
            if len(matrices) != 1:
                continue
            found[(oid,)] = Occurrence(
                (self.instance_component[oid],), (oid,), to_mm(matrices[0][1]), source="instance"
            )
        return list(found.values())

    # -- joints ------------------------------------------------------------

    def joints(self) -> list[Joint]:
        """Joint records: two instances and their joint frames."""
        instances = set(self.instance_component)
        result = []
        for item in self.items:
            if item.size <= MAX_OCCURRENCE_RECORD:
                continue
            names = self.strings(item.oid, 4)
            if "Assemble" not in names:
                continue
            refs = [r for r in self.refs(item.oid) if r in instances]
            if len(refs) < 2:
                continue
            frames = {offset: matrix for offset, matrix in rigid_matrices(self.raw(item.oid))}
            name = names[0] if names else ""
            result.append(Joint(name, (refs[0], refs[1]), frames, tuple(refs)))
        return result

    # -- geometry ----------------------------------------------------------

    def component(self, name: str):
        for component in self.graph.components:
            if component.name == name:
                return component
        raise KeyError(name)

    def solid_vertices(self, blob: str) -> list[np.ndarray]:
        """Vertex positions (mm, component frame) of each solid in a body blob.

        A blob also keeps solids from earlier points of the timeline, so the
        caller decides which solid is the part (the one that matches its STL).
        """
        from ezf3d.asm.brep import Shape

        shape = Shape(self.blobs[blob].model())
        result = []
        for solid in shape.solids():
            points: dict[tuple[float, float, float], None] = {}
            for face in solid.faces():
                for loop in face.loops():
                    for coedge in loop.coedges():
                        edge = coedge.edge
                        if edge is None:
                            continue
                        for vertex in (edge.start, edge.end):
                            if vertex is not None and vertex.position is not None:
                                points[tuple(np.round(vertex.position * CM_TO_MM, 5))] = None
            if points:
                result.append(np.array(list(points)))
        return result


def read_stl(path: str | os.PathLike[str]) -> np.ndarray:
    """Triangle corners of a binary STL as an ``(n, 3)`` array, millimetres."""
    data = open(path, "rb").read()
    count = struct.unpack_from("<I", data, 80)[0]
    record = np.dtype([("normal", "<f4", 3), ("vertices", "<f4", (3, 3)), ("attr", "<u2")])
    triangles = np.frombuffer(data, dtype=record, count=count, offset=84)
    return triangles["vertices"].reshape(-1, 3).astype(float)


def nearest_distances(points: np.ndarray, targets: np.ndarray, chunk: int = 512) -> np.ndarray:
    """Distance from each of ``points`` to the nearest of ``targets`` (brute force, chunked)."""
    result = np.empty(len(points))
    for start in range(0, len(points), chunk):
        block = points[start : start + chunk]
        result[start : start + chunk] = np.sqrt(
            ((block[:, None, :] - targets[None, :, :]) ** 2).sum(-1).min(1)
        )
    return result


#: A B-Rep vertex counts as landing on the STL within this distance (mm).
#: Exported STLs are float32, so exact matches land near 1e-4 mm.
VERTEX_TOLERANCE_MM = 0.05


@dataclass
class Match:
    """How a transformed solid lands on an STL."""

    matched: int
    total: int

    @property
    def fraction(self) -> float:
        return self.matched / self.total if self.total else 0.0


def stl_targets(stl_points: np.ndarray) -> np.ndarray:
    return np.unique(stl_points.round(4), axis=0)


def match(vertices: np.ndarray, to_print: np.ndarray, targets: np.ndarray) -> Match:
    """How many of a solid's vertices land on an STL vertex after ``to_print``."""
    if not len(vertices):
        return Match(0, 0)
    moved = vertices @ to_print[:3, :3].T + to_print[:3, 3]
    return Match(int((nearest_distances(moved, targets) < VERTEX_TOLERANCE_MM).sum()), len(vertices))


def joint_agreement(joint: Joint, placed: dict[int, np.ndarray]) -> float | None:
    """How well a joint agrees with the transforms of the two occurrences it joins.

    A joint holds one frame on each component (plus copies); joined, the two
    frames coincide in the design. Returns the smallest mismatch (mm) over the
    ways of assigning the stored frames to the two components, or ``None`` if
    either occurrence has no transform.
    """
    a, b = joint.instances
    if a not in placed or b not in placed:
        return None
    best = None
    frames = list(joint.frames.items())
    for offset_a, frame_a in frames:
        for offset_b, frame_b in frames:
            if offset_a == offset_b:
                continue
            fa, fb = to_mm(frame_a), to_mm(frame_b)
            error = float(np.abs(placed[a] @ fa - placed[b] @ fb).max())
            best = error if best is None else min(best, error)
    return best
