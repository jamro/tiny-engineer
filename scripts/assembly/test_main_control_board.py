#!/usr/bin/env python3
"""Tests for main_control_board.py (numpy only).

    python3 scripts/assembly/test_main_control_board.py

Also checks that the committed meshes (scripts/assembly/hardware/MainControlBoard.stl,
ESP32-C3-Zero-headers.stl) are what the generator makes from the KiCad design now.
"""

import math
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import main_control_board as board  # noqa: E402


def area(points, triangles) -> float:
    p = np.asarray(points)
    a, b, c = p[triangles[:, 0]], p[triangles[:, 1]], p[triangles[:, 2]]
    return float(np.sum((b[:, 0] - a[:, 0]) * (c[:, 1] - a[:, 1]) - (b[:, 1] - a[:, 1]) * (c[:, 0] - a[:, 0])) / 2)


class Triangulate(unittest.TestCase):
    def test_polygon_with_holes(self):
        square = np.array([[0, 0], [10, 0], [10, 10], [0, 10]], float)
        holes = [board.circle(3, 3, 1, 16), board.circle(7, 6, 1.5, 16)]
        points, triangles = board.triangulate(square, holes)
        hole_area = sum(abs(board.signed_area(h)) for h in holes)
        self.assertAlmostEqual(area(points, triangles), 100 - hole_area, places=6)
        self.assertTrue(all(area(points, t[None]) > 0 for t in triangles), "counter-clockwise triangles")

    def test_concave_polygon(self):
        ell = np.array([[0, 0], [4, 0], [4, 1], [1, 1], [1, 4], [0, 4]], float)
        points, triangles = board.triangulate(ell)
        self.assertAlmostEqual(area(points, triangles), 7.0)

    def test_arc_through_three_points(self):
        pts = board.arc_points((1, 0), (math.sqrt(0.5), math.sqrt(0.5)), (0, 1))
        np.testing.assert_allclose(np.hypot(pts[:, 0], pts[:, 1]), 1.0)
        np.testing.assert_allclose(pts[[0, -1]], [[1, 0], [0, 1]], atol=1e-12)


class Board(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.board = board.Board()

    def test_outline_and_holes(self):
        outline = self.board.outline()
        # 90.5 × 50 mm with the USB-C tab, x 112..202.5 and y 85..135 in KiCad.
        np.testing.assert_allclose(outline.min(0), [112, -135], atol=1e-6)
        np.testing.assert_allclose(outline.max(0), [202.5, -85], atol=1e-6)
        mounting = [h.mean(0) for h in self.board.holes() if abs(np.ptp(h[:, 0]) - 2.2) < 0.05]
        np.testing.assert_allclose(sorted(map(tuple, mounting)), [(126, -95), (136, -115), (183, -115)], atol=1e-6)
        self.assertAlmostEqual(self.board.thickness, 1.6002)

    def test_mounting_holes_land_on_the_designs(self):
        # The design's simplified board has its three M2 holes at these points of its frame.
        mounting = [h.mean(0) for h in self.board.holes() if abs(np.ptp(h[:, 0]) - 2.2) < 0.05]
        design = board.to_design(np.array([[x, y, 0.0] for x, y in mounting]))[:, :2]
        np.testing.assert_allclose(sorted(map(tuple, design.round(6))), [(5, 0.5), (5, 47.5), (25, 57.5)], atol=1e-6)

    def test_meshes_are_up_to_date(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = board.OUT
            try:
                board.OUT = Path(tmp)
                board.main()
            finally:
                board.OUT = out
            for name in ("MainControlBoard.stl", "ESP32-C3-Zero-headers.stl"):
                self.assertEqual((Path(tmp) / name).read_bytes(), (out / name).read_bytes(), f"{name}: run scripts/assembly/main_control_board.py")


class Headers(unittest.TestCase):
    def test_two_rows_of_nine_under_the_module(self):
        corners = board.esp32_headers().corners().reshape(-1, 3)
        self.assertAlmostEqual(corners[:, 2].min(), -board.HEADER_BODY - board.HEADER_PIN_ABOVE)
        self.assertAlmostEqual(corners[:, 2].max(), board.HEADER_PIN_BELOW)
        x0, x1 = board.ESP32_PIN_CENTRE[0] - board.ESP32_ROW_SPACING / 2, board.ESP32_PIN_CENTRE[0] + board.ESP32_ROW_SPACING / 2
        self.assertAlmostEqual(corners[:, 0].min(), x0 - 1.27)
        self.assertAlmostEqual(corners[:, 0].max(), x1 + 1.27)
        self.assertAlmostEqual(np.ptp(corners[:, 1]), 9 * 2.54)


if __name__ == "__main__":
    unittest.main()
