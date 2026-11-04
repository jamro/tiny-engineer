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


def volume(vertices, triangles) -> float:
    v, t = np.asarray(vertices, float), np.asarray(triangles)
    return float((np.cross(v[t[:, 0]], v[t[:, 1]]) * v[t[:, 2]]).sum() / 6)


def closed(triangles) -> bool:
    """Every directed edge has its reverse once: closed and consistently wound."""
    edges = {}
    for t in triangles:
        for a, b in ((t[0], t[1]), (t[1], t[2]), (t[2], t[0])):
            edges[(a, b)] = edges.get((a, b), 0) + 1
    return all(n == 1 and edges.get((b, a)) == 1 for (a, b), n in edges.items())


class Meshes(unittest.TestCase):
    def test_screw_is_a_closed_pan_head_screw(self):
        vertices, triangles = fasteners.screw_mesh(8, 3.5, 1.4, 2.0)
        self.assertTrue(closed(triangles))
        self.assertGreater(volume(vertices, triangles), 0, "outward faces")
        np.testing.assert_allclose(vertices.min(0), [-1.75, -1.75, -8], atol=0.01)
        np.testing.assert_allclose(vertices.max(0), [1.75, 1.75, 1.4], atol=0.01)
        shaft = vertices[(vertices[:, 2] < 0.1) & (vertices[:, 2] > -7.5) & (np.hypot(vertices[:, 0], vertices[:, 1]) < 1.5)]
        self.assertAlmostEqual(np.hypot(shaft[:, 0], shaft[:, 1]).max(), 1.0)

    def test_nut_is_a_closed_hex_nut(self):
        vertices, triangles = fasteners.nut_mesh(4.0, 1.6, 2.0)
        self.assertTrue(closed(triangles))
        area = 2 * math.sqrt(3) * 2.0**2 - math.pi * 1.0**2
        self.assertAlmostEqual(volume(vertices, triangles), area * 1.6, delta=0.15)
        self.assertAlmostEqual(np.abs(vertices[:, 1]).max(), 2.0, places=6)  # flats 4.0 apart

    def test_servo_tabs_have_the_designs_screw_holes(self):
        p = presets.servo_params("sg90")
        # Measured on the design's servo at SG90 (artifacts/assembly/sg90/hardware/Servo.stl).
        np.testing.assert_allclose(presets.servo_holes(p), [(-19.37, 0.0), (8.27, 0.0)], atol=1e-9)
        for preset in ("sg90", "fs0307", "es9051", "hd1370a"):
            vertices, triangles = presets.servo_mesh(presets.servo_params(preset))
            self.assertGreater(volume(vertices, triangles), 0, preset)
        vertices, triangles = presets.holed_block(0, 4, 0, 2, 0, 3, (2, 1.5), 1.0)
        self.assertTrue(closed(triangles))
        self.assertAlmostEqual(volume(vertices, triangles), 4 * 2 * 3 - 2 * math.pi, delta=0.1)


class Geometry(unittest.TestCase):
    def test_ray_hits_every_face_it_crosses(self):
        vertices, triangles = fasteners.nut_mesh(4.0, 1.6, 2.0)
        tris = vertices[triangles]
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
