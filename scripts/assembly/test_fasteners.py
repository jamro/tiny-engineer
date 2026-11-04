#!/usr/bin/env python3
"""Tests for fasteners.py (numpy only).

    python3 scripts/assembly/test_fasteners.py

The placement itself runs on every robot in build.sh, which fails when a line
of fasteners.json finds another count than it says.
"""

import json
import math
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import fasteners  # noqa: E402
import presets  # noqa: E402


class Meshes(unittest.TestCase):
    def test_screw_is_a_closed_pan_head_screw(self):
        mesh = fasteners.screw_mesh(8, 3.5, 1.4, 2.0)
        self.assertTrue(mesh.closed())
        self.assertGreater(mesh.volume(), 0, "outward faces")
        corners = mesh.corners().reshape(-1, 3)
        np.testing.assert_allclose(corners.min(0), [-1.75, -1.75, -8], atol=0.01)
        np.testing.assert_allclose(corners.max(0), [1.75, 1.75, 1.4], atol=0.01)
        shaft = corners[(corners[:, 2] < 0.1) & (corners[:, 2] > -7.5) & (np.hypot(corners[:, 0], corners[:, 1]) < 1.5)]
        self.assertAlmostEqual(np.hypot(shaft[:, 0], shaft[:, 1]).max(), 1.0)

    def test_nut_is_a_closed_hex_nut(self):
        mesh = fasteners.nut_mesh(4.0, 1.6, 2.0)
        self.assertTrue(mesh.closed())
        hole = 24 / 2 * math.sin(2 * math.pi / 24)  # the 24-gon's area, r = 1
        self.assertAlmostEqual(mesh.volume(), (2 * math.sqrt(3) * 2.0**2 - hole) * 1.6, places=6)
        self.assertAlmostEqual(np.abs(mesh.corners()[:, :, 1]).max(), 2.0, places=6)  # flats 4.0 apart


class Geometry(unittest.TestCase):
    def test_ray_hits_every_face_it_crosses(self):
        tris = fasteners.nut_mesh(4.0, 1.6, 2.0).corners()
        # Through the nut's body: in at the bottom, out at the top.
        np.testing.assert_allclose(fasteners.ray_hits(tris, np.array([1.5, 0, -1.0]), np.array([0, 0, 1.0])), [1.0, 2.6])
        # Down the hole: nothing.
        self.assertEqual(len(fasteners.ray_hits(tris, np.array([0, 0, -1.0]), np.array([0, 0, 1.0]))), 0)

    def test_frame_puts_z_along_up(self):
        up = np.array([0.0, -1.0, 0.0])
        M = fasteners.frame(np.array([1.0, 2.0, 3.0]), up, 0.3)
        np.testing.assert_allclose(M[:3, :3] @ M[:3, :3].T, np.eye(3), atol=1e-12)
        self.assertAlmostEqual(np.linalg.det(M[:3, :3]), 1.0)
        np.testing.assert_allclose(M[:3, 2], up)
        np.testing.assert_allclose(M[:3, 3], [1, 2, 3])

    def test_spans_pair_a_parts_circles(self):
        line = {"circles": [("Chest", 1.05, s, None) for s in (-48.2, -43.3, -40.3, -33.8)] + [("Chest", 2.0, -50.2, None), ("Belly", 1.05, 0.0, None)]}
        self.assertEqual(fasteners.spans(line, "Chest", {1.05}), [(-48.2, -43.3), (-40.3, -33.8)])
        self.assertTrue(fasteners.inside(-45.0, fasteners.spans(line, "Chest", {1.05})))
        self.assertFalse(fasteners.inside(-42.0, fasteners.spans(line, "Chest", {1.05})))
        self.assertAlmostEqual(fasteners.overlap((-32.2, -48.2), [(-48.2, -43.3), (-40.3, -33.8)]), 4.9 + 6.5)


class Config(unittest.TestCase):
    def test_every_line_names_a_step_of_the_build(self):
        config = json.loads(fasteners.CONFIG.read_text())
        titles = fasteners.shown_by_step(json.loads(fasteners.STEPS.read_text()))
        for spec in config["screws"]:
            self.assertIn(spec["step"], titles, spec["id"])
            if "nut" in spec:
                self.assertIn(spec["nut"]["step"], titles, spec["id"])
        self.assertEqual(len({spec["id"] for spec in config["screws"]}), len(config["screws"]))

    def test_the_list_matches_the_bom(self):
        # 3d_models/README.md#screws, less what the 3D model does not draw: the breakout boards
        # of the modular build (4 × M2×4, 2 × M2×4, 4 × M2×16 and 4 nuts for PCA9685,
        # MAX98357A, USB-C) and the optional second screw of the neck servo.
        config = json.loads(fasteners.CONFIG.read_text())
        count = {}
        for spec in config["screws"]:
            count[spec["length"]] = count.get(spec["length"], 0) + spec["count"]
        self.assertEqual(count, {4: 8 - 6, 8: 27 - 1, 16: 15 - 4})
        self.assertEqual(sum(1 for spec in config["screws"] if "nut" in spec), 6 - 4)


if __name__ == "__main__":
    unittest.main()
