#!/usr/bin/env python3
"""Tests for presets.py (numpy only).

    python3 scripts/assembly/test_presets.py              # unit tests
    python3 scripts/assembly/test_presets.py --history    # also solve the two designs saved at
                                                          # different presets (needs git history and ezf3d)

With --history, the design saved at SG90 (268ad4b) is solved at FS0307 and the
design saved at FS0307 (22afc31) at SG90, each with the STEP exports of its
commit; every occurrence must land where the other design has it, to 0.001 mm.
"""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import presets  # noqa: E402

HISTORY = ("268ad4b", "sg90"), ("22afc31", "fs0307")


class Servo(unittest.TestCase):
    def test_levels_match_the_designs_servos(self):
        # Measured on the servo body of the design saved at SG90 and at FS0307.
        xs, ys, zs = presets.servo_levels(presets.servo_params("sg90"))
        self.assertEqual([round(v, 3) for v in xs.values()], [-22.0, -17.2, -9.4, 0.0, 6.1, 10.9])
        self.assertEqual([round(v, 3) for v in ys.values()], [0.6, -3.0, -6.5, -10.9, -12.4, -13.9, -30.9])
        self.assertAlmostEqual(zs["front"], 6.4)
        xs, ys, zs = presets.servo_levels(presets.servo_params("fs0307"))
        self.assertEqual([round(v, 3) for v in xs.values()], [-19.6, -15.8, -9.3, 0.0, 4.8, 8.6])
        self.assertEqual([round(v, 3) for v in ys.values()], [0.6, -3.0, -7.0, -10.7, -11.5, -12.3, -25.3])
        self.assertAlmostEqual(zs["front"], 4.6)

    def test_transfer_follows_named_levels(self):
        src = presets.servo_levels(presets.servo_params("sg90"))
        dst = presets.servo_levels(presets.servo_params("fs0307"))
        np.testing.assert_allclose(presets.servo_transfer(np.array([10.9, -13.9, -6.4]), src, dst), [8.6, -12.3, -4.6])
        # not a level: kept (the shaft top, an offset)
        np.testing.assert_allclose(presets.servo_transfer(np.array([0.0, 0.6, 0.3]), src, dst), [0.0, 0.6, 0.3])

    def test_mesh_spans_the_servo(self):
        vertices, triangles = presets.servo_mesh(presets.servo_params("fs0307"))
        np.testing.assert_allclose(vertices.min(0), [-19.6, -25.3, -4.6], atol=1e-9)
        np.testing.assert_allclose(vertices.max(0), [8.6, 0.6, 4.6], atol=1e-9)
        # closed: every edge is shared by exactly two triangles
        edges = {}
        for t in triangles:
            for a, b in ((t[0], t[1]), (t[1], t[2]), (t[2], t[0])):
                edges[tuple(sorted((a, b)))] = edges.get(tuple(sorted((a, b))), 0) + 1
        self.assertTrue(all(n == 2 for n in edges.values()))


def box(at, size):
    at = np.asarray(at, float)
    return [("v", np.round(at + np.array(c) * size, 4)) for c in np.ndindex(2, 2, 2)]


class Features(unittest.TestCase):
    def test_rigid_shift(self):
        a, b = box([0, 0, 0], [2, 3, 4]), box([5, 0, 1], [2, 3, 4])
        np.testing.assert_allclose(presets.rigid_shift(a, b), [5, 0, 1])
        self.assertIsNone(presets.rigid_shift(a, box([0, 0, 0], [2, 3, 5])))

    def test_patch_follows_the_features_around_a_point(self):
        # Two blocks; the far one moves by (0, 0, 2) and grows, the near one moves by (1, 0, 0).
        src = box([0, 0, 0], [2, 2, 2]) + box([20, 0, 0], [2, 2, 2])
        dst = box([1, 0, 0], [2, 2, 2]) + box([20, 0, 2], [3, 3, 3])
        shift, n, of = presets.PartMap(src, dst).shift_at(np.array([1.0, 1.0, 1.0]), 8.0)
        np.testing.assert_allclose(shift, [1, 0, 0])
        self.assertEqual((n, of), (8, 8))


def history_available():
    try:
        for commit, _ in HISTORY:
            subprocess.run(["git", "cat-file", "-e", f"{commit}:3d_models/cad/TinyEngineer.f3d"], check=True, capture_output=True, cwd=presets.ROOT)
        import ezf3d  # noqa: F401
        return True
    except Exception:
        return False


@unittest.skipUnless("--history" in sys.argv and history_available(), "needs --history, git history and ezf3d")
class History(unittest.TestCase):
    def test_solves_each_saved_design_at_the_other_preset(self):
        from f3d_assembly import Design

        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            for commit, preset in HISTORY:
                (tmp / commit).mkdir()
                (tmp / f"{commit}.f3d").write_bytes(subprocess.run(["git", "show", f"{commit}:3d_models/cad/TinyEngineer.f3d"], check=True, capture_output=True, cwd=presets.ROOT).stdout)
                for p in {preset for _, preset in HISTORY}:
                    archive = subprocess.run(["git", "archive", commit, f"3d_models/parts/{p}/step"], check=True, capture_output=True, cwd=presets.ROOT).stdout
                    subprocess.run(["tar", "-x", "-C", str(tmp / commit)], input=archive, check=True)
            for (src, src_preset), (dst, dst_preset) in (HISTORY, HISTORY[::-1]):
                solver = presets.Solver(tmp / f"{src}.f3d", src_preset, dst_preset,
                                        tmp / src / "3d_models/parts" / src_preset / "step", tmp / dst / "3d_models/parts" / dst_preset / "step")
                placed, _ = solver.solve()
                truth = {o.instances[-1]: o.matrix for o in Design(tmp / f"{dst}.f3d").occurrences() if not o.in_print_layout and o.source != "identity"}
                self.assertEqual(set(placed), set(truth))
                for inst, matrix in placed.items():
                    self.assertLess(np.abs(matrix - truth[inst]).max(), 1e-3, f"{src_preset} -> {dst_preset}: {solver.occ[inst].component}")


if __name__ == "__main__":
    unittest.main(argv=[a for a in sys.argv if a != "--history"])
