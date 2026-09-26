# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Rebuild and inspect HD1370A bottom cover with CadQuery; dimensions in mm."""
from pathlib import Path
import json, math
import cadquery as cq
import trimesh
from underside_profile import derive_profile, wire_from_segments

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "3d_models/parts/hd1370a/bottom-cover"
SOURCES = ROOT / "3d_models/parts/hd1370a/step"
PARAMS = {
    "thickness": 4.5,
    "clearance": 0.3,
    "hole_diameter": 2.3,
    "head_diameter": 4.3,
    "head_depth": 2.3,
}
HOLES = [(-30, -61.6), (26, -61.6), (-30, 61.6), (26, 61.6)]


def common_volume(a, b):
    return a.intersect(b).Volume()


def mesh(s):
    v, f = s.tessellate(0.025, 0.1)
    return trimesh.Trimesh(vertices=[p.toTuple() for p in v], faces=f, process=True)


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    desk = (
        cq.importers.importStep(str(SOURCES / "Desk.step"))
        .val()
        .rotate((0, 0, 0), (1, 0, 0), 180)
        .translate((0, 0, 38.5))
    )
    chair = (
        cq.importers.importStep(str(SOURCES / "Chair.step"))
        .val()
        .translate((-59.3, 0, 4.5))
    )
    source_wire, segments, source_edges, chair_rear = derive_profile(desk, chair)
    (Path(__file__).parent / "underside-profile.json").write_text(
        json.dumps(
            {
                "revision": 2,
                "source": "Actual bottom planar rim of assembled HD1370A Desk and Chair",
                "perimeter_offset_mm": 0,
                "segments": segments,
            },
            indent=2,
        )
        + "\n"
    )
    outline = wire_from_segments(segments)
    p = cq.Workplane("XY").newObject(
        [cq.Solid.extrudeLinear(outline, [], cq.Vector(0, 0, PARAMS["thickness"]))]
    )
    for x, y in HOLES:
        p = p.cut(
            cq.Workplane("XY")
            .center(x, y)
            .circle(PARAMS["hole_diameter"] / 2)
            .extrude(PARAMS["thickness"])
        )
        p = p.cut(
            cq.Workplane("XY")
            .center(x, y)
            .circle(PARAMS["head_diameter"] / 2)
            .extrude(PARAMS["head_depth"])
        )
    plate = p.val()
    cq.exporters.export(plate, str(OUT.parent / "step/BottomCover.step"))
    # Print top mating face on bed; counterbores open upward, with no bridges.
    printable = plate.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, 4.5))
    m = mesh(printable)
    m.metadata["units"] = "mm"
    m.export(str(OUT.parent / "stl/BottomCover.stl"))
    (OUT.parent / "3mf/BottomCover.3mf").write_bytes(
        trimesh.exchange.threemf.export_3MF(trimesh.Scene(m))
    )
    desk = (
        cq.importers.importStep(str(SOURCES / "Desk.step"))
        .val()
        .rotate((0, 0, 0), (1, 0, 0), 180)
        .translate((0, 0, 38.5))
    )
    chair = (
        cq.importers.importStep(str(SOURCES / "Chair.step"))
        .val()
        .translate((-59.3, 0, 4.5))
    )
    top = (
        cq.importers.importStep(str(SOURCES / "DeskTop.step"))
        .val()
        .translate((0, 0, 40.0))
    )
    pad = (
        cq.importers.importStep(str(SOURCES / "DeskPad.step"))
        .val()
        .translate((-2.0, 0.0000082699, 38.5))
    )
    ass = cq.Assembly(name="BottomCover_FitCheck_HD1370A")
    for n, s, c in [
        ("BottomCover", plate, (0.08, 0.6, 0.72)),
        ("Desk", desk, (0.35, 0.35, 0.38)),
        ("Chair", chair, (0.48, 0.48, 0.5)),
        ("DeskTop", top, (0.28, 0.28, 0.3)),
        ("DeskPad", pad, (0.65, 0.5, 0.28)),
    ]:
        ass.add(s, name=n, color=cq.Color(*c))
    ass.export(str(OUT / "BottomCover_FitCheck.step"))
    # Normalize exporter whitespace without changing STEP records.
    for path in [
        OUT.parent / "step/BottomCover.step",
        OUT / "BottomCover_FitCheck.step",
    ]:
        path.write_text(
            "\n".join(line.rstrip() for line in path.read_text().splitlines()) + "\n"
        )
    report = {
        "parameters_mm": PARAMS,
        "outline_segments_mm": segments,
        "revision": 2,
        "hole_centres_mm": HOLES,
        "scope": "HD1370A only; CAD verified, physical print not yet fit-tested",
        "assembly_transforms": {
            "Desk": "rotate X 180 deg; translate (0,0,38.5) mm",
            "Chair": "translate (-59.3,0,4.5) mm",
            "DeskTop": "translate (0,0,40.0) mm",
            "DeskPad": "translate (-2,0.0000082699,38.5) mm",
        },
        "valid_solid": plate.isValid(),
        "solid_count": len(plate.Solids()),
        "mesh_watertight": m.is_watertight,
        "mesh_winding_consistent": m.is_winding_consistent,
        "mesh_euler_number": int(m.euler_number),
        "volume_mm3": plate.Volume(),
        "bounds_mm": m.bounds.tolist(),
        "part_interference_mm3": {},
        "head_checks": [],
    }
    for n, s in [("Desk", desk), ("Chair", chair), ("DeskTop", top), ("DeskPad", pad)]:
        report["part_interference_mm3"][n] = common_volume(plate, s)
    # M2 conservative envelope from native source: 4 mm diameter x 2 mm head.
    for x, y in HOLES:
        head = cq.Solid.makeCylinder(2.0, 2.0, cq.Vector(x, y, 0.3))
        shaft = cq.Solid.makeCylinder(1.0, 6.0, cq.Vector(x, y, 2.3))
        report["head_checks"].append(
            {
                "centre_mm": [x, y],
                "head_to_floor_clearance_mm": 0.3,
                "radial_head_clearance_mm": 0.15,
                "radial_shank_clearance_mm": 0.15,
                "remaining_plate_bearing_mm": 2.2,
                "M2x6_insertion_into_desk_mm": 3.8,
                "head_plate_interference_mm3": common_volume(plate, head),
                "shank_plate_interference_mm3": common_volume(plate, shaft),
                "shank_desk_interference_mm3": common_volume(desk, shaft),
            }
        )
    # Compare the finished cover's actual mating-face exterior to the source
    # underside, independently of the chosen hole geometry.
    mating = (
        max(
            [
                f
                for f in plate.Faces()
                if abs(f.BoundingBox().zmin - 4.5) < 1e-6
                and abs(f.BoundingBox().zmax - 4.5) < 1e-6
            ],
            key=lambda f: f.Area(),
        )
        .outerWire()
        .translate((0, 0, -4.5))
    )
    expected_face = cq.Face.makeFromWires(source_wire)
    actual_face = cq.Face.makeFromWires(mating)
    overhang = actual_face.cut(expected_face).Area()
    undercoverage = expected_face.cut(actual_face).Area()
    dev = max(
        mating.distance(cq.Vertex.makeVertex(*e.positionAt(i / 100).toTuple()))
        for e in source_wire.Edges()
        for i in range(101)
    )
    report["perimeter_fit"] = {
        "perimeter_offset_mm": 0,
        "excess_footprint_area_mm2": overhang,
        "missing_footprint_area_mm2": undercoverage,
        "max_sampled_source_edge_deviation_mm": dev,
        "copied_desk_edge_count": len(source_edges),
        "radii_mm": [s["radius_mm"] for s in segments if s["type"] == "CIRCLE"],
        "square_rear_corners_mm": [[-75.3, -35.6], [-75.3, 35.6]],
        "rear_seam_bridges_mm": [0.3, 0.3],
        "note": "Straight rear closure coincides with chair and desk rear planes and bridges only their two existing 0.3 mm assembly seams.",
    }
    assert overhang < 1e-6 and undercoverage < 1e-6 and dev < 1e-6
    # Exact STEP round trip plus print-3MF checks.
    reread = cq.importers.importStep(str(OUT.parent / "step/BottomCover.step")).val()
    scene = trimesh.load_scene(OUT.parent / "3mf/BottomCover.3mf")
    report["step_roundtrip_valid"] = reread.isValid()
    report["step_roundtrip_volume_error_mm3"] = abs(reread.Volume() - plate.Volume())
    report["3mf_mesh_count"] = len(scene.geometry)
    report["3mf_bounds_mm"] = scene.bounds.tolist()
    assert (
        plate.isValid()
        and len(plate.Solids()) == 1
        and m.is_watertight
        and m.euler_number == -6
    )
    assert all(abs(v) < 1e-6 for v in report["part_interference_mm3"].values())
    assert all(
        abs(h[k]) < 1e-6
        for h in report["head_checks"]
        for k in [
            "head_plate_interference_mm3",
            "shank_plate_interference_mm3",
            "shank_desk_interference_mm3",
        ]
    )
    (OUT / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return plate, desk, chair


if __name__ == "__main__":
    build()
