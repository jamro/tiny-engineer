# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Run in Fusion to update the optional HD1370A reference in the master archive.

The independent parametric cover remains the editable source. This reference is
hidden by default and outside PRINT_LAYOUT, so another servo's batch exports
cannot accidentally include this fixed HD1370A part. Original parameters and
geometry are checked before the master file is overwritten.
"""
import json
import traceback
from pathlib import Path

import adsk.core
import adsk.fusion

ROOT = Path(__file__).resolve().parents[3]
MASTER = ROOT / "3d_models/cad/TinyEngineer.f3d"
OUT = ROOT / "3d_models/parts/hd1370a/bottom-cover"
NAME = "OPTIONAL_BOTTOM_COVER_HD1370A"


def original_geometry(design):
    result = {}
    for occurrence in design.rootComponent.allOccurrences:
        if occurrence.component.name == NAME:
            continue
        bodies = []
        for body in occurrence.bRepBodies:
            box = body.preciseBoundingBox
            bodies.append(
                [body.volume * 1000]
                + [
                    v * 10
                    for p in [box.minPoint, box.maxPoint]
                    for v in [p.x, p.y, p.z]
                ]
            )
        result[occurrence.fullPathName] = bodies
    return result


def main():
    app = adsk.core.Application.get()
    document = app.importManager.importToNewDocument(
        app.importManager.createFusionArchiveImportOptions(str(MASTER))
    )
    document.name = "TinyEngineer master with optional HD1370A cover"
    design = adsk.fusion.Design.cast(app.activeProduct)
    root = design.rootComponent
    for old in list(root.occurrences):
        if old.component.name == NAME:
            old.deleteMe()
    baseline = original_geometry(design)
    parameters = {p.name: p.expression for p in design.userParameters}
    options = app.importManager.createSTEPImportOptions(
        str(OUT.parent / "step/BottomCover.step")
    )
    added = app.importManager.importToTarget2(options, root)
    occurrences = [
        o for o in added if o.objectType == adsk.fusion.Occurrence.classType()
    ]
    assert len(occurrences) == 1
    cover = occurrences[0]
    cover.component.name = NAME
    transform = adsk.core.Matrix3D.create()
    transform.translation = adsk.core.Vector3D.create(
        4.35, -1.39259877217, -1.54740122783
    )
    cover.transform2 = transform
    cover.isLightBulbOn = False
    if design.snapshots.hasPendingSnapshot:
        design.snapshots.add()
    assert original_geometry(design) == baseline

    report = {
        "scope": "Optional HD1370A reference in current upstream master",
        "thread_retention_verified": False,
        "hd1370a_interference": {},
        "original_geometry_identical_before_save": original_geometry(design)
        == baseline,
        "original_user_parameters_preserved": {
            p.name: p.expression for p in design.userParameters
        }
        == parameters,
        "default_servo_expression": parameters["servo_id"],
        "optional_component_hidden": not cover.isLightBulbOn,
        "original_occurrence_count": len(baseline),
        "outside_print_layout": cover.assemblyContext is None,
    }
    assert report["original_geometry_identical_before_save"]
    assert report["original_user_parameters_preserved"]
    assert report["outside_print_layout"]
    assert design.exportManager.execute(
        design.exportManager.createFusionArchiveExportOptions(str(MASTER))
    )
    document.close(False)
    reopened = app.importManager.importToNewDocument(
        app.importManager.createFusionArchiveImportOptions(str(MASTER))
    )
    design = adsk.fusion.Design.cast(app.activeProduct)
    root = design.rootComponent
    cover = next(o for o in root.occurrences if o.component.name == NAME)
    assert not cover.isLightBulbOn
    assert {p.name: p.expression for p in design.userParameters} == parameters
    after = original_geometry(design)
    assert baseline.keys() == after.keys()
    errors = []
    for name, bodies in baseline.items():
        assert len(bodies) == len(after[name])
        for old, new in zip(bodies, after[name]):
            errors.extend(abs(a - b) for a, b in zip(old, new))
    report["max_original_geometry_numeric_change_after_reopen"] = max(errors, default=0)
    assert max(errors, default=0) < 0.001
    report["master_archive_reopened"] = True
    report["cover_volume_mm3"] = cover.bRepBodies.item(0).volume * 1000

    # Check the HD1370A preset in the reopened inspection copy only. Never save
    # preset regeneration into the master: its original default stays untouched.
    presets = json.loads(
        (ROOT / "3d_models/fusion/TinyEngineerTools/servos.json").read_text()
    )
    preset = next(p for p in presets.values() if p["servo_id"] == "hd1370a")
    dimensions = [k for k in preset if k != "servo_id"]
    design.userParameters.itemByName("servo_id").expression = "'hd1370a'"
    assert design.modifyParameters(
        [design.userParameters.itemByName(k) for k in dimensions],
        [adsk.core.ValueInput.createByString(preset[k]) for k in dimensions],
    )
    design.computeAll()
    for name in ["Desk", "Chair", "DeskTop", "DeskPad"]:
        part = next(o for o in root.occurrences if o.component.name == name)
        objects = adsk.core.ObjectCollection.create()
        objects.add(cover)
        objects.add(part)
        request = design.createInterferenceInput(objects)
        request.areCoincidentFacesIncluded = False
        result = design.analyzeInterference(request)
        report["hd1370a_interference"][name] = result.count if result else 0
    assert all(n == 0 for n in report["hd1370a_interference"].values())
    temporary = adsk.fusion.TemporaryBRepManager.get()
    native_parts = []
    for name in ["Desk", "Chair"]:
        occurrence = next(o for o in root.occurrences if o.component.name == name)
        native_parts.append((name, [temporary.copy(b) for b in occurrence.bRepBodies]))
    reopened.close(False)
    # Short-lived export documents are removed after the independent STEP is saved.
    for name, bodies in native_parts:
        doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
        model = adsk.fusion.Design.cast(app.activeProduct)
        model.designType = adsk.fusion.DesignTypes.DirectDesignType
        for body in bodies:
            model.rootComponent.bRepBodies.add(body)
        assert model.exportManager.execute(
            model.exportManager.createSTEPExportOptions(
                str(OUT / ("native-master-" + name + ".step"))
            )
        )
        doc.close(False)
    (OUT / "master-integration-verification.json").write_text(
        json.dumps(report, indent=2) + "\n"
    )
    (OUT / "master-integration-error.txt").unlink(missing_ok=True)
    print("Master updated; original defaults and geometry preserved.")


try:
    main()
except Exception:
    (OUT / "master-integration-error.txt").write_text(traceback.format_exc())
    print(traceback.format_exc())
