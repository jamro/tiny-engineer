# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
import adsk.core, adsk.fusion, json, traceback
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
O = ROOT / "3d_models/parts/hd1370a/bottom-cover"
C = ROOT / "3d_models/cad/bottom-cover"


def col(xs):
    c = adsk.core.ObjectCollection.create()
    for x in xs:
        c.add(x)
    return c


def check(d, a, b):
    i = d.createInterferenceInput(col([a, b]))
    i.areCoincidentFacesIncluded = False
    r = d.analyzeInterference(i)
    return r.count if r else 0


try:
    app = adsk.core.Application.get()
    srcdoc = next(
        d
        for d in app.documents
        if d.name.startswith("TinyEngineer HD1370A with Bottom Cover v2")
    )
    srcdoc.activate()
    d = adsk.fusion.Design.cast(app.activeProduct)
    cover = next(
        o
        for o in d.rootComponent.allOccurrences
        if o.component.name == "BottomCover_HD1370A"
    )
    out = {
        "revision": 2,
        "thread_retention_verified": False,
        "full_original_assembly_interference": {},
    }
    for o in d.rootComponent.occurrences:
        if o.component.name in ["Desk", "Chair", "DeskTop", "DeskPad"]:
            out["full_original_assembly_interference"][o.component.name] = check(
                d, cover, o
            )
    fitdoc = next(
        doc
        for doc in app.documents
        if doc.name.startswith("BottomCover Fit Check HD1370A v2")
    )
    fitdoc.activate()
    d = adsk.fusion.Design.cast(app.activeProduct)
    root = d.rootComponent
    cover = next(o for o in root.allOccurrences if o.component.name == "BottomCover")
    desk = next(o for o in root.allOccurrences if o.component.name == "Desk")
    # Geometric screw envelopes; not printed parts and not helical thread models.
    out["screw_envelopes"] = []
    for idx, (x, y) in enumerate(
        [(-30, -61.6), (26, -61.6), (-30, 61.6), (26, 61.6)], 1
    ):
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = "M2x6 head envelope " + str(idx) + " (REFERENCE ONLY)"
        tm = adsk.fusion.TemporaryBRepManager.get()
        a = adsk.core.Point3D.create(x / 10, y / 10, 0.03)
        b = adsk.core.Point3D.create(x / 10, y / 10, 0.23)
        c = adsk.core.Point3D.create(x / 10, y / 10, 0.83)
        h = tm.createCylinderOrCone(a, 0.2, b, 0.2)
        s = tm.createCylinderOrCone(b, 0.1, c, 0.1)
        tm.booleanOperation(h, s, adsk.fusion.BooleanTypes.UnionBooleanType)
        bf = occ.component.features.baseFeatures.add()
        bf.startEdit()
        body = occ.component.bRepBodies.add(h, bf)
        bf.finishEdit()
        body.name = "M2x6 envelope - do not print"
        out["screw_envelopes"].append(
            {
                "head_floor_gap_mm": body.preciseBoundingBox.minPoint.z * 10,
                "cover_interference": check(d, cover, occ),
                "desk_interference": check(d, desk, occ),
            }
        )
    # Save screw reference assembly, default visible to inspect recesses.
    app.activeViewport.refresh()
    assert d.exportManager.execute(
        d.exportManager.createFusionArchiveExportOptions(
            str(C / "BottomCover_FitCheck_HD1370A.f3d")
        )
    )
    out["all_geometric_checks_pass"] = all(
        v == 0 for v in out["full_original_assembly_interference"].values()
    ) and all(
        r["cover_interference"] == 0
        and r["desk_interference"] == 0
        and r["head_floor_gap_mm"] > 0
        for r in out["screw_envelopes"]
    )
    assert out["all_geometric_checks_pass"], out
    (O / "fusion-final-checks.json").write_text(json.dumps(out, indent=2) + "\n")
    # Print export from native parametric Fusion body, transformed in a separate local document.
    standalone = next(
        doc
        for doc in app.documents
        if doc.name.startswith("BottomCover HD1370A v2")
        and doc.products.itemByProductType(
            "DesignProductType"
        ).rootComponent.bRepBodies.count
        == 1
    )
    standalone.activate()
    model = adsk.fusion.Design.cast(app.activeProduct)
    tm = adsk.fusion.TemporaryBRepManager.get()
    s = tm.copy(model.rootComponent.bRepBodies.item(0))
    import math

    t = adsk.core.Matrix3D.create()
    t.setToRotation(
        math.pi, adsk.core.Vector3D.create(1, 0, 0), adsk.core.Point3D.create(0, 0, 0)
    )
    t.translation = adsk.core.Vector3D.create(0, 0, 0.45)
    tm.transform(s, t)
    printdoc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    printdoc.name = "BottomCover print orientation"
    pd = adsk.fusion.Design.cast(app.activeProduct)
    pd.designType = adsk.fusion.DesignTypes.DirectDesignType
    b = pd.rootComponent.bRepBodies.add(s)
    b.name = "BottomCover"
    ex = pd.exportManager
    opt = ex.createC3MFExportOptions(b, str(O.parent / "3mf/BottomCover.3mf"))
    opt.sendToPrintUtility = False
    opt.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
    opt.surfaceDeviation = 0.0005
    opt.normalDeviation = math.radians(3)
    assert ex.execute(opt)
    printdoc.close(False)
    reopened = app.importManager.importToNewDocument(
        app.importManager.createFusionArchiveImportOptions(
            str(C / "BottomCover_HD1370A.f3d")
        )
    )
    rd = adsk.fusion.Design.cast(app.activeProduct)
    out["native_f3d_reopened"] = {
        "body_count": rd.rootComponent.bRepBodies.count,
        "volume_mm3": rd.rootComponent.bRepBodies.item(0).volume * 1000,
        "timeline_count": rd.timeline.count,
    }
    (O / "fusion-final-checks.json").write_text(json.dumps(out, indent=2) + "\n")
    reopened.close(False)
    fitdoc.activate()
    err = O / "fusion-build-error.txt"
    if err.exists():
        err.unlink()
except:
    (O / "fusion-final-error.txt").write_text(traceback.format_exc())
    print(traceback.format_exc())
