# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Independent STEP comparison and all-corner inspection sheet."""
from pathlib import Path
import cadquery as cq
import json, numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from underside_profile import derive_profile, bottom_faces

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "3d_models/parts/hd1370a/bottom-cover"
SRC = ROOT / "3d_models/parts/hd1370a/step"
desk = (
    cq.importers.importStep(str(SRC / "Desk.step"))
    .val()
    .rotate((0, 0, 0), (1, 0, 0), 180)
    .translate((0, 0, 38.5))
)
chair = (
    cq.importers.importStep(str(SRC / "Chair.step")).val().translate((-59.3, 0, 4.5))
)
expected, _, _, _ = derive_profile(desk, chair)
cover = cq.importers.importStep(str(OUT / "BottomCover_Fusion.step")).val()
actual = (
    max(bottom_faces(cover, 4.5), key=lambda f: f.Area())
    .outerWire()
    .translate((0, 0, -4.5))
)
ref = cq.importers.importStep(str(OUT.parent / "step/BottomCover.step")).val()
result = {
    "revision": 2,
    "fusion_vs_cadquery_symmetric_difference_mm3": cover.cut(ref).Volume()
    + ref.cut(cover).Volume(),
    "fusion_perimeter_excess_area_mm2": cq.Face.makeFromWires(actual)
    .cut(cq.Face.makeFromWires(expected))
    .Area(),
    "fusion_perimeter_missing_area_mm2": cq.Face.makeFromWires(expected)
    .cut(cq.Face.makeFromWires(actual))
    .Area(),
    "fusion_max_sampled_boundary_deviation_mm": max(
        actual.distance(cq.Vertex.makeVertex(*e.positionAt(i / 100).toTuple()))
        for e in expected.Edges()
        for i in range(101)
    ),
}
assert all(abs(v) < 1e-6 for k, v in result.items() if k != "revision"), result
(OUT / "perimeter-verification.json").write_text(json.dumps(result, indent=2) + "\n")
fig, axs = plt.subplots(3, 3, figsize=(13, 13), layout="constrained")
views = [
    ("Rear left - square", (-79, -68), (29, 39)),
    ("Left shoulder - R3 / R1", (-37, -19), (32, 45)),
    ("Desk left rear - R3", (-37, -24), (56, 69)),
    ("Rear right - square", (-79, -68), (-39, -29)),
    ("Complete underside", (-83, 37), (-72, 72)),
    ("Desk left front - R3", (20, 34), (56, 69)),
    ("Right shoulder - R3 / R1", (-37, -19), (-45, -32)),
    ("Desk right rear - R3", (-37, -24), (-69, -56)),
    ("Desk right front - R3", (20, 34), (-69, -56)),
]
for ax, (title, xlim, ylim) in zip(axs.flat, views):
    for wire, color, ls, lw, label in [
        (actual, "#007e9c", "-", 3, "Cover edge"),
        (expected, "#db6b25", "--", 1.5, "Original rim / rear closure"),
    ]:
        for i, e in enumerate(wire.Edges()):
            arr = np.array([e.positionAt(t).toTuple() for t in np.linspace(0, 1, 201)])
            ax.plot(
                arr[:, 0],
                arr[:, 1],
                color=color,
                linestyle=ls,
                linewidth=lw,
                label=label if i == 0 else None,
            )
    ax.set(xlim=xlim, ylim=ylim, title=title)
    ax.set_aspect("equal")
    ax.grid(alpha=0.15)
    ax.tick_params(labelsize=8)
    if title == "Complete underside":
        ax.scatter([-30, 26, -30, 26], [-61.6, -61.6, 61.6, 61.6], s=9, color="#777777")
fig.suptitle(
    "HD1370A bottom cover v2 | Exact underside perimeter\nAll dimensions in mm - zero CAD overhang or undercoverage",
    fontsize=16,
)
handles, labels = axs[0, 0].get_legend_handles_labels()
fig.legend(handles, labels, loc="outside lower center", ncol=2, frameon=False)
fig.savefig(OUT / "perimeter-corner-check.png", dpi=150)
print(json.dumps(result, indent=2))
