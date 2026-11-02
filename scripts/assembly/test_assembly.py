#!/usr/bin/env python3
"""Unit tests for the assembly extractor's matrix helpers (numpy only, no ezf3d).

    python3 scripts/assembly/test_assembly.py
"""

import struct
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from f3d_assembly import Joint, Match, inverse, joint_agreement, match, rigid_matrices, stl_targets, to_mm  # noqa: E402


def rotation_z(degrees: float) -> np.ndarray:
    a = np.radians(degrees)
    return np.array([[np.cos(a), -np.sin(a), 0], [np.sin(a), np.cos(a), 0], [0, 0, 1]])


def transform(rotation: np.ndarray, translation) -> np.ndarray:
    result = np.eye(4)
    result[:3, :3] = rotation
    result[:3, 3] = translation
    return result


def fusion_bytes(matrix: np.ndarray) -> bytes:
    """A transform as Fusion writes it: 16 doubles, translation in the last row."""
    return struct.pack("<16d", *matrix.T.reshape(-1))


class RigidMatrices(unittest.TestCase):
    def test_finds_matrix_at_any_alignment(self):
        expected = transform(rotation_z(30), [1.5, -2.0, 7.25])
        for padding in range(8):
            raw = b"\x07" * padding + fusion_bytes(expected) + b"\x00" * 5
            found = rigid_matrices(raw)
            self.assertEqual(len(found), 1, padding)
            offset, matrix = found[0]
            self.assertEqual(offset, padding)
            np.testing.assert_allclose(matrix, expected)

    def test_accepts_translation_in_last_column(self):
        expected = transform(rotation_z(-90), [0, 3, 0])
        found = rigid_matrices(struct.pack("<16d", *expected.reshape(-1)))
        np.testing.assert_allclose(found[0][1], expected)

    def test_ignores_non_rigid(self):
        scaled = transform(np.eye(3) * 2, [0, 0, 0])
        self.assertEqual(rigid_matrices(fusion_bytes(scaled)), [])


class Transforms(unittest.TestCase):
    def test_inverse(self):
        m = transform(rotation_z(42), [3, 4, 5])
        np.testing.assert_allclose(inverse(m) @ m, np.eye(4), atol=1e-12)

    def test_to_mm_scales_translation_only(self):
        m = to_mm(transform(rotation_z(90), [1, 2, 3]))
        np.testing.assert_allclose(m[:3, 3], [10, 20, 30])
        np.testing.assert_allclose(m[:3, :3], rotation_z(90), atol=1e-12)


class Matching(unittest.TestCase):
    def test_match_counts_vertices_on_the_stl(self):
        vertices = np.array([[0, 0, 0], [10, 0, 0], [0, 10, 0], [0, 0, 10.0]])
        to_print = transform(rotation_z(90), [5, 0, 0])
        stl = vertices @ to_print[:3, :3].T + to_print[:3, 3]
        stl = np.vstack([stl[:3], [[99, 99, 99]]])  # one vertex moved
        result = match(vertices, to_print, stl_targets(stl))
        self.assertEqual((result.matched, result.total), (3, 4))
        self.assertEqual(Match(3, 4).fraction, 0.75)

    def test_joint_agreement(self):
        # Component A sits 10 mm along x from B; the joint frame is at A's origin
        # and at x = 10 mm in B's frame.
        placed = {1: transform(np.eye(3), [10, 0, 0]), 2: np.eye(4)}
        frames = {83: transform(np.eye(3), [0, 0, 0]), 251: transform(np.eye(3), [1.0, 0, 0])}  # cm
        self.assertAlmostEqual(joint_agreement(Joint("j", (1, 2), frames), placed), 0.0)
        wrong = {1: transform(np.eye(3), [12, 0, 0]), 2: np.eye(4)}
        self.assertGreater(joint_agreement(Joint("j", (1, 2), frames), wrong), 1.0)
        self.assertIsNone(joint_agreement(Joint("j", (1, 3), frames), placed))


if __name__ == "__main__":
    unittest.main()
