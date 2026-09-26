# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Check native master mating parts against the unchanged HD1370A fixture.

Run after integrate_master_in_fusion.py. Uses volume, topology counts and
bidirectional edge samples; coincident STEP booleans are unreliable for these
complex imported parts. This is a sampled comparison, not a boolean proof.
"""
import json
from pathlib import Path

import cadquery as cq

ROOT = Path(__file__).resolve().parents[3]
PARTS = ROOT / "3d_models/parts/hd1370a"
OUT = PARTS / "bottom-cover"


def sampled_edge_distance(source, target):
    return max(
        target.distance(cq.Vertex.makeVertex(*edge.positionAt(i / 10).toTuple()))
        for edge in source.Edges()
        for i in range(11)
    )


report_path = OUT / "master-integration-verification.json"
report = json.loads(report_path.read_text())
report["native_master_mate_comparison"] = {}
for name in ["Desk", "Chair"]:
    native = cq.importers.importStep(str(OUT / f"native-master-{name}.step")).val()
    fixture = cq.importers.importStep(str(PARTS / "step" / f"{name}.step")).val()
    if name == "Desk":
        fixture = fixture.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, 38.5))
    else:
        fixture = fixture.translate((-59.3, 0, 4.5))
    fixture = fixture.translate((43.5, -13.9259877217, -15.4740122783))
    result = {
        "valid_solids": native.isValid() and fixture.isValid(),
        "volume_difference_mm3": abs(native.Volume() - fixture.Volume()),
        "face_counts": [len(native.Faces()), len(fixture.Faces())],
        "edge_counts": [len(native.Edges()), len(fixture.Edges())],
        "native_to_fixture_max_sampled_edge_distance_mm": sampled_edge_distance(
            native, fixture
        ),
        "fixture_to_native_max_sampled_edge_distance_mm": sampled_edge_distance(
            fixture, native
        ),
        "samples_per_edge": 11,
    }
    assert result["valid_solids"]
    assert result["volume_difference_mm3"] < 1e-6
    assert result["face_counts"][0] == result["face_counts"][1]
    assert result["edge_counts"][0] == result["edge_counts"][1]
    assert result["native_to_fixture_max_sampled_edge_distance_mm"] < 1e-6
    assert result["fixture_to_native_max_sampled_edge_distance_mm"] < 1e-6
    report["native_master_mate_comparison"][name] = result
report_path.write_text(json.dumps(report, indent=2) + "\n")
# These inspection exports duplicate existing sources and are not deliverables.
for name in ["Desk", "Chair"]:
    (OUT / f"native-master-{name}.step").unlink()
print(json.dumps(report["native_master_mate_comparison"], indent=2))
