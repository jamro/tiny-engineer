#!/usr/bin/env python3
"""Tests for boards.py and meshes.py (numpy only).

    python3 scripts/assembly/test_boards.py

Also checks that the committed meshes (scripts/assembly/hardware/*.stl) are
what boards.py makes now.
"""

import math
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import boards  # noqa: E402
import meshes  # noqa: E402
from f3d_assembly import read_stl  # noqa: E402


def area(points, triangles) -> float:
    p = np.asarray(points)
    a, b, c = p[triangles[:, 0]], p[triangles[:, 1]], p[triangles[:, 2]]
    return float(np.sum((b[:, 0] - a[:, 0]) * (c[:, 1] - a[:, 1]) - (b[:, 1] - a[:, 1]) * (c[:, 0] - a[:, 0])) / 2)


class Meshes(unittest.TestCase):
    def test_polygon_with_holes(self):
        square = np.array([[0, 0], [10, 0], [10, 10], [0, 10]], float)
        holes = [meshes.circle(3, 3, 1, 16), meshes.circle(7, 6, 1.5, 16)]
        points, triangles = meshes.triangulate(square, holes)
        self.assertAlmostEqual(area(points, triangles), 100 - sum(abs(meshes.signed_area(h)) for h in holes), places=6)
        self.assertTrue(all(area(points, t[None]) > 0 for t in triangles), "counter-clockwise triangles")

    def test_concave_polygon(self):
        ell = np.array([[0, 0], [4, 0], [4, 1], [1, 1], [1, 4], [0, 4]], float)
        points, triangles = meshes.triangulate(ell)
        self.assertAlmostEqual(area(points, triangles), 7.0)

    def test_prism_with_a_hole_is_closed(self):
        mesh = meshes.prism(np.array([[0, 0], [4, 0], [4, 3], [0, 3]], float), 0.0, 2.0, (1, 0, 0), holes=[meshes.circle(2, 1.5, 1.0, 32)])
        self.assertTrue(mesh.closed())
        self.assertAlmostEqual(mesh.volume(), (12 - 16 * math.sin(math.pi / 16)) * 2, places=6)
        turned = meshes.along_y(mesh)
        self.assertAlmostEqual(turned.volume(), mesh.volume(), places=9)
        np.testing.assert_allclose(turned.corners().reshape(-1, 3).max(0), [4, 2, 0])

    def test_arc_through_three_points(self):
        pts = meshes.arc_points((1, 0), (math.sqrt(0.5), math.sqrt(0.5)), (0, 1))
        np.testing.assert_allclose(np.hypot(pts[:, 0], pts[:, 1]), 1.0)
        np.testing.assert_allclose(pts[[0, -1]], [[1, 0], [0, 1]], atol=1e-12)

    def test_stl_keeps_colours(self):
        with tempfile.TemporaryDirectory() as tmp:
            mesh = meshes.box(0, 1, 0, 1, 0, 1, (1.0, 0.0, 0.0))
            path = Path(tmp) / "box.stl"
            self.assertEqual(mesh.write(path), 12)
            data = path.read_bytes()
            self.assertEqual(int.from_bytes(data[84 + 48 : 84 + 50], "little"), 0x8000 | (31 << 10))
            np.testing.assert_allclose(read_stl(path).reshape(-1, 3, 3), mesh.corners(), atol=1e-6)


class MainControlBoard(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.board = boards.Board()

    def mounting(self):
        return [h.mean(0) for h in self.board.holes() if abs(np.ptp(h[:, 0]) - 2.2) < 0.05]

    def test_outline_and_holes(self):
        outline = self.board.outline()
        # 90.5 × 50 mm with the USB-C tab, x 112..202.5 and y 85..135 in KiCad.
        np.testing.assert_allclose(outline.min(0), [112, -135], atol=1e-6)
        np.testing.assert_allclose(outline.max(0), [202.5, -85], atol=1e-6)
        np.testing.assert_allclose(sorted(map(tuple, self.mounting())), [(126, -95), (136, -115), (183, -115)], atol=1e-6)
        self.assertAlmostEqual(self.board.thickness, 1.6002)

    def test_mounting_holes_land_on_the_designs(self):
        # The design's simplified board has its three M2 holes at these points of its frame.
        design = boards.to_design(np.array([[x, y, 0.0] for x, y in self.mounting()]))[:, :2]
        np.testing.assert_allclose(sorted(map(tuple, design.round(6))), [(5, 0.5), (5, 47.5), (25, 57.5)], atol=1e-6)


class Esp32(unittest.TestCase):
    def test_module_follows_the_drawing(self):
        mesh = boards.esp32_c3_zero()
        corners = mesh.corners().reshape(-1, 3)
        # 18 × 23.5 mm PCB, USB-C 1.58 mm past its end, 3.2 mm tall on the 1 mm PCB.
        np.testing.assert_allclose(corners.min(0), [0, 0, 0], atol=1e-9)
        np.testing.assert_allclose(corners.max(0), [18, 25.08, 4.2], atol=1e-9)
        pins = np.array(boards.esp32_pins())
        self.assertEqual(len(pins), 18)
        np.testing.assert_allclose(sorted(set(pins[:, 0])), [1.38, 16.62])
        np.testing.assert_allclose([pins[:, 1].min(), pins[:, 1].max()], [1.59, 21.91])

    def test_two_rows_of_nine_headers_under_the_module(self):
        corners = boards.esp32_headers().corners().reshape(-1, 3)
        self.assertAlmostEqual(corners[:, 2].min(), -boards.HEADER_BODY - boards.HEADER_PIN_ABOVE)
        self.assertAlmostEqual(corners[:, 2].max(), boards.HEADER_PIN_BELOW)
        np.testing.assert_allclose([corners[:, 0].min(), corners[:, 0].max()], [1.38 - 1.27, 16.62 + 1.27])
        self.assertAlmostEqual(np.ptp(corners[:, 1]), 9 * 2.54)


class Committed(unittest.TestCase):
    def test_meshes_are_up_to_date(self):
        with tempfile.TemporaryDirectory() as tmp:
            boards.main(Path(tmp))
            for name in boards.outputs():
                self.assertEqual((Path(tmp) / name).read_bytes(), (boards.OUT / name).read_bytes(), f"{name}: run scripts/assembly/boards.py")


if __name__ == "__main__":
    unittest.main()
