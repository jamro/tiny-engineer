# SPDX-FileCopyrightText: 2026 Hanson Wen
# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Copy the real planar rim. No offset, projected overhang, or blanket fillets."""
import cadquery as cq

TOL = 1e-6


def bottom_faces(shape, z):
    return [
        f
        for f in shape.Faces()
        if abs(f.BoundingBox().zmin - z) < TOL and abs(f.BoundingBox().zmax - z) < TOL
    ]


def key(v):
    return tuple(round(c, 6) for c in v.toTuple())


def derive_profile(desk, chair, z=4.5):
    rim = max(bottom_faces(desk, z), key=lambda f: f.Area())
    edges = rim.outerWire().Edges()
    pts = [e.startPoint() for e in edges] + [e.endPoint() for e in edges]
    xmin = min(p.x for p in pts)
    back = [p for p in pts if abs(p.x - xmin) < TOL]
    a = max(back, key=lambda p: p.y)
    b = min(back, key=lambda p: p.y)
    # The original wall is an open C. Of the two rim paths between rear
    # corners, the exterior is the one reaching the foremost desk wall.
    paths = []
    for first in [
        e for e in edges if key(a) in (key(e.startPoint()), key(e.endPoint()))
    ]:
        path = []
        current = a
        edge = first
        used = set()
        while True:
            used.add(edges.index(edge))
            path.append(edge)
            current = (
                edge.endPoint()
                if key(current) == key(edge.startPoint())
                else edge.startPoint()
            )
            if key(current) == key(b):
                break
            edge = next(
                e
                for i, e in enumerate(edges)
                if i not in used
                and key(current) in (key(e.startPoint()), key(e.endPoint()))
            )
        paths.append(path)
    exterior = max(paths, key=lambda es: max(e.BoundingBox().xmax for e in es))
    # The chair rear rim shares exactly the same X plane as the desk tips.
    chair_rear = [
        e
        for f in bottom_faces(chair, z)
        for e in f.outerWire().Edges()
        if e.geomType() == "LINE"
        and abs(e.startPoint().x - xmin) < TOL
        and abs(e.endPoint().x - xmin) < TOL
    ]
    assert chair_rear, "Chair rear plane must agree with the desk tips"
    assert abs(sum(e.Length() for e in chair_rear) - 60.6) < TOL
    closure = cq.Edge.makeLine(b, a)
    original = cq.Wire.assembleEdges(exterior + [closure]).translate((0, 0, -z))
    # Export line/three-point-arc data for a native editable Fusion sketch.
    segments = []
    current = a
    for edge in exterior + [closure]:
        end = (
            edge.endPoint()
            if key(current) == key(edge.startPoint())
            else edge.startPoint()
        )
        item = {
            "type": edge.geomType(),
            "start_mm": [round(current.x, 9), round(current.y, 9)],
            "end_mm": [round(end.x, 9), round(end.y, 9)],
        }
        if edge.geomType() == "CIRCLE":
            mid = edge.positionAt(0.5)
            item["mid_mm"] = [round(mid.x, 9), round(mid.y, 9)]
            item["radius_mm"] = round(edge.radius(), 9)
        segments.append(item)
        current = end
    return original, segments, exterior, chair_rear


def wire_from_segments(segments):
    edges = []
    for s in segments:
        a = cq.Vector(*s["start_mm"], 0)
        b = cq.Vector(*s["end_mm"], 0)
        edges.append(
            cq.Edge.makeLine(a, b)
            if s["type"] == "LINE"
            else cq.Edge.makeThreePointArc(a, cq.Vector(*s["mid_mm"], 0), b)
        )
    return cq.Wire.assembleEdges(edges)
