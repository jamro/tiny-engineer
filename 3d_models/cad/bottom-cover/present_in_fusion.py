# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
import adsk.core, adsk.fusion, math, traceback
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
C = ROOT / "3d_models/cad/bottom-cover"
O = ROOT / "3d_models/parts/hd1370a/bottom-cover"
app = adsk.core.Application.get()


def pt(xs):
    return adsk.core.Point3D.create(*[x / 10 for x in xs])


def view(eye, target, up, path, size=None):
    cam = app.activeViewport.camera
    cam.isSmoothTransition = False
    cam.cameraType = adsk.core.CameraTypes.OrthographicCameraType
    cam.eye = pt(eye)
    cam.target = pt(target)
    cam.upVector = adsk.core.Vector3D.create(*up)
    cam.isFitView = size is None
    if size:
        cam.setExtents(size / 10, size / 10)
    app.activeViewport.camera = cam
    app.activeViewport.refresh()
    app.activeViewport.saveAsImageFile(str(O / path), 1400, 1000)


try:
    standalone = next(
        doc for doc in app.documents if doc.name.startswith("BottomCover HD1370A v2")
    )
    standalone.activate()
    model = adsk.fusion.Design.cast(app.activeProduct)
    tm = adsk.fusion.TemporaryBRepManager.get()
    s = tm.copy(model.rootComponent.bRepBodies.item(0))
    t = adsk.core.Matrix3D.create()
    t.setToRotation(
        math.pi, adsk.core.Vector3D.create(1, 0, 0), adsk.core.Point3D.create(0, 0, 0)
    )
    t.translation = adsk.core.Vector3D.create(0, 0, 0.45)
    tm.transform(s, t)
    pd = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    d = adsk.fusion.Design.cast(app.activeProduct)
    d.designType = adsk.fusion.DesignTypes.DirectDesignType
    b = d.rootComponent.bRepBodies.add(s)
    b.name = "BottomCover"
    ex = d.exportManager
    opt = ex.createC3MFExportOptions(b, str(O.parent / "3mf/BottomCover.3mf"))
    opt.sendToPrintUtility = False
    opt.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
    opt.surfaceDeviation = 0.0005
    opt.normalDeviation = math.radians(3)
    assert ex.execute(opt)
    pd.close(False)
    fit = next(
        doc
        for doc in app.documents
        if doc.name.startswith("BottomCover Fit Check HD1370A v2")
    )
    fit.activate()
    d = adsk.fusion.Design.cast(app.activeProduct)
    for label, target, delta in [
        ("rear-left", (-75.3, 35.6, 4.5), (-40, 40, 18)),
        ("rear-right", (-75.3, -35.6, 4.5), (-40, -40, 18)),
        ("desk-left-front", (26, 61.6, 4.5), (40, 40, 18)),
        ("desk-right-front", (26, -61.6, 4.5), (40, -40, 18)),
        ("desk-left-rear", (-30, 61.6, 4.5), (-40, 40, 18)),
        ("desk-right-rear", (-30, -61.6, 4.5), (-40, -40, 18)),
        ("shoulder-left", (-28, 38, 4.5), (-40, 40, 10)),
        ("shoulder-right", (-28, -38, 4.5), (-40, -40, 10)),
    ]:
        view(
            tuple(a + b for a, b in zip(target, delta)),
            target,
            (0, 0, 1),
            "corner-" + label + ".png",
            22,
        )
    view((-170, -180, -170), (-23, 0, 12), (0, 0, 1), "fit-with-recessed-screws.png")
    view((150, -180, 130), (-23, 0, 35), (0, 0, 1), "fit-isometric.png")
    assert d.exportManager.execute(
        d.exportManager.createFusionArchiveExportOptions(
            str(C / "BottomCover_FitCheck_HD1370A.f3d")
        )
    )
    app.userInterface.palettes.itemById("TextCommands").isVisible = False
except:
    (O / "fusion-presentation-error.txt").write_text(traceback.format_exc())
    print(traceback.format_exc())
