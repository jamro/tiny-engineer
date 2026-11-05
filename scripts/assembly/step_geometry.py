"""STEP / print-frame helpers shared by fasteners (and formerly presets)."""

from __future__ import annotations

import re
from pathlib import Path

import numpy as np

from extract import matrix_of
from f3d_assembly import inverse

MIRRORED = {"UpperArm(Mirror)": "UpperArm"}


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


def step_to_print_rotation(entry: dict) -> np.ndarray:
    """A placed part's rotation from its component (STEP) frame to its STL's print frame."""
    return (inverse(matrix_of(entry)) @ matrix_of(entry, "occurrence"))[:3, :3]


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
