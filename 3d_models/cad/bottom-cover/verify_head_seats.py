# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
from pathlib import Path
import cadquery as cq
import trimesh, numpy as np, json
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon, Rectangle

O = Path(__file__).resolve().parents[3] / "3d_models/parts/hd1370a/bottom-cover"
plate = cq.importers.importStep(str(O / "BottomCover_Fusion.step")).val()
centres = [(-30, -61.6), (26, -61.6), (-30, 61.6), (26, 61.6)]
checks = []
for x, y in centres:
    head = cq.Solid.makeCylinder(2, 2, cq.Vector(x, y, 0.3))
    pushed = head.translate((0, 0, 0.1))
    checks.append(
        {
            "centre_mm": [x, y],
            "seated_head_collision_mm3": plate.intersect(head).Volume(),
            "head_pushed_up_0p1mm_collision_mm3": plate.intersect(pushed).Volume(),
        }
    )
assert all(
    c["seated_head_collision_mm3"] < 1e-6
    and c["head_pushed_up_0p1mm_collision_mm3"] > 0.8
    for c in checks
)
m = trimesh.load_scene(O.parent / "3mf/BottomCover.3mf").to_mesh()
# Rotate print coordinates back into the installed orientation.
m.apply_transform(trimesh.transformations.rotation_matrix(np.pi, [1, 0, 0]))
m.apply_translation([0, 0, 4.5])
x, y = centres[2]
section = m.section(plane_origin=[0, y, 0], plane_normal=[0, 1, 0])
fig, ax = plt.subplots(figsize=(12, 7), layout="constrained")
for line in section.discrete:
    ax.add_patch(
        Polygon(
            np.column_stack((line[:, 0] - x, line[:, 2])),
            closed=True,
            facecolor="#12839a",
            edgecolor="#075769",
            lw=1.5,
        )
    )
# Conservative envelope explicitly distinguished from a detailed screw model.
ax.add_patch(
    Rectangle((-2, 0.3), 4, 2, facecolor="#a8adb4", edgecolor="#333b45", lw=1.5)
)
ax.add_patch(
    Rectangle((-1, 2.3), 2, 6.0, facecolor="#a8adb4", edgecolor="#333b45", lw=1.5)
)
ax.axhline(0, color="#b45a21", lw=2)
ax.axhline(4.5, color="#777", ls=":", lw=1)
ax.annotate(
    "Plastic shoulder stops the head",
    xy=(-1.6, 2.3),
    xytext=(-6.9, 6.3),
    arrowprops={"arrowstyle": "->", "color": "#1e4b5a"},
    fontsize=12,
    color="#163b48",
)
ax.annotate(
    "2.2 mm of plastic above the head seat",
    xy=(2.5, 3.4),
    xytext=(3.1, 5.7),
    arrowprops={"arrowstyle": "->", "color": "#1e4b5a"},
    fontsize=11,
    color="#163b48",
)
ax.annotate(
    "Shaft clearance: 2.3 mm",
    xy=(-1.15, 3.5),
    xytext=(-6.9, 4.6),
    arrowprops={"arrowstyle": "->", "color": "#1e4b5a"},
    fontsize=11,
)
ax.annotate(
    "Head recess: 4.3 mm wide\n2.3 mm deep",
    xy=(2.15, 1.15),
    xytext=(3.1, 1.3),
    arrowprops={"arrowstyle": "->", "color": "#1e4b5a"},
    fontsize=11,
)
ax.annotate(
    "Head stays 0.3 mm above the desk",
    xy=(0, 0.15),
    xytext=(-6.9, -1.5),
    arrowprops={"arrowstyle": "->", "color": "#1e4b5a"},
    fontsize=11,
)
ax.text(3.1, 0.15, "Flat underside / desk surface", color="#9b4a1b", fontsize=10)
ax.text(
    0,
    8.9,
    "Shaft continues into the robot\nThread grip is a separate fit requirement",
    ha="center",
    fontsize=11,
)
ax.set(
    xlim=(-7.2, 9),
    ylim=(-2.2, 10),
    aspect="equal",
    xlabel="Distance across one hole (mm)",
    ylabel="Height above the flat underside (mm)",
)
ax.set_title(
    "Actual printable file: cross-section through one screw hole\nTeal = printed cover; grey = 4 mm head / 2 mm shaft reference envelope",
    loc="left",
    fontsize=14,
)
ax.grid(alpha=0.10)
fig.savefig(O / "screw-seat-section.png", dpi=160)
(O / "screw-seat-check.json").write_text(
    json.dumps(
        {
            "all_four_head_shoulders_present": True,
            "checks": checks,
            "through_hole_mm": 2.3,
            "head_recess_mm": 4.3,
            "recess_depth_mm": 2.3,
            "plastic_above_head_seat_mm": 2.2,
            "reference_head_overlap_per_side_mm": 0.85,
            "scope": "Checks head retention against the cover only. Desk pilot thread engagement depends on actual screw and printed hole size; zero collision does not establish grip.",
        },
        indent=2,
    )
    + "\n"
)
print(json.dumps(checks, indent=2))
