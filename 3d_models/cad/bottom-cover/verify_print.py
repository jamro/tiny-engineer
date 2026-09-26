# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Check the native Fusion print export, including recess orientation."""
import trimesh, json, numpy as np
from pathlib import Path

p = Path(__file__).resolve().parents[3] / "3d_models/parts/hd1370a/bottom-cover"
s = trimesh.load_scene(p.parent / "3mf/BottomCover.3mf")
m = s.to_mesh()
out = {
    "revision": 2,
    "mesh_count": len(s.geometry),
    "bounds_mm": s.bounds.tolist(),
    "extents_mm": s.extents.tolist(),
    "watertight": m.is_watertight,
    "winding_consistent": m.is_winding_consistent,
    "euler_number": int(m.euler_number),
    "volume_mm3": float(m.volume),
    "units": "mm",
    "sections": [],
}
for z in [0.5, 3.5]:
    sec = m.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
    flat, mat = sec.to_2D()
    polys = sorted(flat.polygons_closed, key=lambda x: x.area)
    holes = polys[:4]
    dias = [float(2 * np.sqrt(poly.area / np.pi)) for poly in holes]
    out["sections"].append(
        {"height_from_print_bed_mm": z, "hole_equivalent_diameters_mm": dias}
    )
    assert len(polys) == 5
    assert all(abs(v - (2.3 if z == 0.5 else 4.3)) < 0.01 for v in dias), dias
assert (
    len(s.geometry) == 1
    and m.is_watertight
    and m.is_winding_consistent
    and m.euler_number == -6
)
assert np.allclose(s.extents, [104.3, 129.2, 4.5], atol=1e-4)
(p / "print-verification.json").write_text(json.dumps(out, indent=2) + "\n")
print(json.dumps(out, indent=2))
