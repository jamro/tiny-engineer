# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Run in Fusion (Python.RunScript) to build from the checked-in source assembly.
Creates a parametric cover, then imports the inspected desk/chair fit assembly.
Original TinyEngineer.f3d is never overwritten. All dimensions below are mm.
"""
import adsk.core, adsk.fusion, json, traceback
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "3d_models/parts/hd1370a/bottom-cover"
CAD = ROOT / "3d_models/cad/bottom-cover"


def pt(x, y, z=0):
    return adsk.core.Point3D.create(x / 10, y / 10, z / 10)


def val(s):
    return adsk.core.ValueInput.createByString(s)


def collect(items):
    c = adsk.core.ObjectCollection.create()
    for item in items:
        c.add(item)
    return c


def extrude(comp, profile, distance, operation, name):
    x = comp.features.extrudeFeatures.createInput(profile, operation)
    x.setOneSideExtent(
        adsk.fusion.DistanceExtentDefinition.create(val(distance)),
        adsk.fusion.ExtentDirections.PositiveExtentDirection,
    )
    f = comp.features.extrudeFeatures.add(x)
    f.name = name
    return f


def camera(app, eye, target, up, path):
    cam = app.activeViewport.camera
    cam.isSmoothTransition = False
    cam.cameraType = adsk.core.CameraTypes.OrthographicCameraType
    cam.eye = pt(*eye)
    cam.target = pt(*target)
    cam.upVector = adsk.core.Vector3D.create(*up)
    cam.isFitView = True
    app.activeViewport.camera = cam
    app.activeViewport.refresh()
    app.activeViewport.saveAsImageFile(str(path), 1400, 1000)


def main():
    app = adsk.core.Application.get()
    source_doc = app.importManager.importToNewDocument(
        app.importManager.createFusionArchiveImportOptions(
            str(ROOT / "3d_models/cad/TinyEngineer.f3d")
        )
    )
    source = adsk.fusion.Design.cast(app.activeProduct)
    presets = json.loads(
        (ROOT / "3d_models/fusion/TinyEngineerTools/servos.json").read_text()
    )
    preset = next(p for p in presets.values() if p["servo_id"] == "hd1370a")
    source.userParameters.itemByName("servo_id").expression = "'hd1370a'"
    dims = [
        (source.userParameters.itemByName(k), val(v))
        for k, v in preset.items()
        if k != "servo_id"
    ]
    assert source.modifyParameters([x[0] for x in dims], [x[1] for x in dims])
    source.computeAll()
    for occ in source.rootComponent.occurrences:
        if occ.component.name in [
            "ServoSizingTester",
            "ScrewSizingTest",
            "PRINT_LAYOUT",
            "ASSEMBLY_GUIDE",
        ]:
            occ.isLightBulbOn = False
    source_doc.name = "TinyEngineer HD1370A with Bottom Cover v2"
    for occ in list(source.rootComponent.occurrences):
        if occ.component.name == "OPTIONAL_BOTTOM_COVER_HD1370A":
            occ.deleteMe()
    doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    doc.name = "BottomCover HD1370A v2"
    d = adsk.fusion.Design.cast(app.activeProduct)
    d.designType = adsk.fusion.DesignTypes.ParametricDesignType
    d.unitsManager.distanceDisplayUnits = (
        adsk.fusion.DistanceUnits.MillimeterDistanceUnits
    )
    root = d.rootComponent
    for n, v, c in [
        ("plate_thickness", "4.5 mm", "Overall plate thickness"),
        ("clearance", "0.3 mm", "Original robot CAD clearance"),
        ("screw_diameter", "2 mm", "M2 nominal shaft diameter"),
        ("screw_head_diameter", "4 mm", "Original CAD head envelope"),
        ("screw_head_height", "2 mm", "Original CAD head height envelope"),
    ]:
        d.userParameters.add(n, val(v), "mm", c)
    s = root.sketches.add(root.xYConstructionPlane)
    s.name = "Exact underside rim - R3 desk, R1 shoulders, square chair"
    profile = json.loads((CAD / "underside-profile.json").read_text())
    for seg in profile["segments"]:
        if seg["type"] == "LINE":
            curve = s.sketchCurves.sketchLines.addByTwoPoints(
                pt(*seg["start_mm"]), pt(*seg["end_mm"])
            )
        else:
            curve = s.sketchCurves.sketchArcs.addByThreePoints(
                pt(*seg["start_mm"]), pt(*seg["mid_mm"]), pt(*seg["end_mm"])
            )
        curve.isFixed = True
    assert s.profiles.count == 1
    f = extrude(
        root,
        s.profiles.item(0),
        "plate_thickness",
        adsk.fusion.FeatureOperations.NewBodyFeatureOperation,
        "Plate - exact flush rim and flat floor",
    )
    body = f.bodies.item(0)
    body.name = "BottomCover"
    holes = [(-30, -61.6), (26, -61.6), (-30, 61.6), (26, 61.6)]
    for label, diam, depth in [
        ("M2 through holes", "screw_diameter + clearance", "plate_thickness"),
        (
            "Recessed M2 heads on floor side",
            "screw_head_diameter + clearance",
            "screw_head_height + clearance",
        ),
    ]:
        sk = root.sketches.add(root.xYConstructionPlane)
        sk.name = label
        for x, y in holes:
            circle = sk.sketchCurves.sketchCircles.addByCenterRadius(pt(x, y), 0.1)
            circle.centerSketchPoint.isFixed = True
            sk.sketchDimensions.addDiameterDimension(
                circle, pt(x + 5, y + 5)
            ).parameter.expression = diam
        extrude(
            root,
            collect([sk.profiles.item(i) for i in range(sk.profiles.count)]),
            depth,
            adsk.fusion.FeatureOperations.CutFeatureOperation,
            label,
        )
        sk.isVisible = False
    s.isVisible = False
    d.computeAll()
    body = root.bRepBodies.item(0)
    report = {
        "revision": 2,
        "outline_source": "underside-profile.json; actual underside edges with zero offset",
        "body_count": root.bRepBodies.count,
        "is_solid": body.isSolid,
        "volume_mm3": body.volume * 1000,
        "timeline_features": d.timeline.count,
        "feature_health": [
            {
                "name": f.name,
                "state": int(f.healthState),
                "message": f.errorOrWarningMessage,
            }
            for f in root.features
        ],
    }
    ex = d.exportManager
    assert ex.execute(ex.createSTEPExportOptions(str(OUT / "BottomCover_Fusion.step")))
    step_path = OUT / "BottomCover_Fusion.step"
    step_path.write_text(
        "\n".join(line.rstrip() for line in step_path.read_text().splitlines()) + "\n"
    )
    camera(
        app, (100, -140, -170), (-23, 0, 2.25), (0, 1, 0), OUT / "cover-underside.png"
    )
    assert ex.execute(
        ex.createFusionArchiveExportOptions(str(CAD / "BottomCover_HD1370A.f3d"))
    )
    # Preserve the native source assembly, adding the CAD-generated cover at its exact global frame.
    source_doc.activate()
    opts = app.importManager.createSTEPImportOptions(
        str(OUT.parent / "step/BottomCover.step")
    )
    added = app.importManager.importToTarget2(opts, source.rootComponent)
    new_occ = [x for x in added if x.objectType == adsk.fusion.Occurrence.classType()]
    if not new_occ:
        # Import can return bodies; put them in a named occurrence so alignment remains inspectable.
        occ = source.rootComponent.occurrences.addNewComponent(
            adsk.core.Matrix3D.create()
        )
        for x in added:
            if x.objectType == adsk.fusion.BRepBody.classType():
                x.moveToComponent(occ)
        new_occ = [occ]
    for o in new_occ:
        o.component.name = "BottomCover_HD1370A"
        t = adsk.core.Matrix3D.create()
        t.translation = adsk.core.Vector3D.create(4.35, -1.39259877217, -1.54740122783)
        o.transform2 = t
    if source.snapshots.hasPendingSnapshot:
        source.snapshots.add()
    source.computeAll()
    # Import the actual desk/chair STEP solids, already rotated from print coordinates and aligned.
    fitdoc = app.importManager.importToNewDocument(
        app.importManager.createSTEPImportOptions(
            str(OUT / "BottomCover_FitCheck.step")
        )
    )
    fitdoc.name = "BottomCover Fit Check HD1370A v2"
    fit = adsk.fusion.Design.cast(app.activeProduct)
    report["fit_occurrences"] = [
        {"path": o.fullPathName, "name": o.component.name}
        for o in fit.rootComponent.allOccurrences
    ]
    occs = list(fit.rootComponent.allOccurrences)
    cover = [
        o
        for o in occs
        if o.component.name.startswith("BottomCover") and o.bRepBodies.count
    ][0]
    report["interference"] = {}
    for name in ["Desk", "Chair", "DeskTop", "DeskPad"]:
        obj = [o for o in occs if o.component.name == name][0]
        inp = fit.createInterferenceInput(collect([cover, obj]))
        inp.areCoincidentFacesIncluded = False
        results = fit.analyzeInterference(inp)
        report["interference"][name] = results.count if results else 0
    report["source_global_offset_mm"] = [43.5, -13.9259877217, -15.4740122783]
    (OUT / "fusion-verification.json").write_text(json.dumps(report, indent=2) + "\n")
    assert fit.exportManager.execute(
        fit.exportManager.createFusionArchiveExportOptions(
            str(CAD / "BottomCover_FitCheck_HD1370A.f3d")
        )
    )
    camera(app, (150, -180, 130), (-23, 0, 35), (0, 0, 1), OUT / "fit-isometric.png")
    camera(app, (-170, -180, -170), (-23, 0, 12), (0, 0, 1), OUT / "fit-underside.png")
    camera(app, (160, 0, 13), (-23, 0, 13), (0, 0, 1), OUT / "fit-front.png")
    camera(app, (-23, -250, 20), (-23, 0, 20), (0, 0, 1), OUT / "fit-side.png")
    camera(app, (-23, 0, -230), (-23, 0, 0), (1, 0, 0), OUT / "fit-bottom.png")
    camera(app, (150, -180, 130), (-23, 0, 35), (0, 0, 1), OUT / "fit-isometric.png")
    if (OUT / "fusion-build-error.txt").exists():
        (OUT / "fusion-build-error.txt").unlink()
    print("Native cover, full assembly and fit check exported.")


try:
    main()
except:
    (OUT / "fusion-build-error.txt").write_text(traceback.format_exc())
    print(traceback.format_exc())
