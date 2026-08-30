# 3D printing and mechanical build

Software lives in repos; hardware lives in **enclosures**. 3D printing is how you turn CAD into a physical body — layer by layer, with tolerances that bite if you ignore them. If you've never printed, this chapter is the missing semester: materials, fit, CAD source, and not destroying a servo by forcing it into a tight pocket. **Tiny Engineer** is the case study; the skills transfer to any printed mount or case.

---

## FDM printing in 60 seconds

**FDM** (Fused Deposition Modeling) — the common hobby printer type — melts plastic filament layer by layer.

Knobs that matter:

| Parameter | Typical starting point | What it affects |
| --- | --- | --- |
| **Layer height** | **0.16 mm** (used for the tested prints) | Surface quality vs speed |
| **Infill** | 20–30% structural | Strength vs print time |
| **Orientation** | As in the `.3mf` | Overhangs; supports (see below) |
| **Material** | **PLA** is enough; PETG also works | Ease vs toughness |

### Supports

Plastic cannot print in mid-air. An **overhang** (a surface that sticks out with nothing under it) needs **supports** — extra throwaway plastic the slicer builds like scaffolding, then you snap or dissolve off.

Supports leave scars, use filament, and make small holes and servo pockets messy.

**This project's parts are designed and oriented so they print with no supports.** Keep the `.3mf` orientation. Rotate a part "to look nicer on the bed" and you may suddenly need supports and get ugly undersides.

### What printer

All parts were printed and test-fitted on a **Creality Ender 3 V3 SE**. You should be able to print them on **any FDM printer** that can run PLA (or PETG) at typical hobby settings — the files are not locked to that machine.

---

## Materials: PLA vs PETG

**PLA is sufficient** for the whole robot — desk use, indoor, this design. **PETG also works** if that's what you have (tougher, a bit stringier to print). Pick one spool if you care about color match.

| Material | Use here |
| --- | --- |
| **PLA** | Default. Fine for servo pockets, chair, desk, props |
| **PETG** | Also fine. Slightly stronger; not required |

Antenna zone: keep plastic thin/light near the ESP32 ceramic antenna. No metal paint, no foil wrap "for aesthetics."

---

## Tolerance and "it doesn't fit"

The models are **prepared with clearance** so printed parts and hardware (servos, screws) are meant to **fit together** without sanding as the happy path. 3D printing is still approximate — holes shrink, first layers bulge ("elephant's foot"), printers vary.

**If a servo doesn't slide in:**
- Don't force it — gears aren't meant to take press-fit stress
- Ream with a drill bit by hand, or
- Sand lightly, or
- Open the pocket in CAD and re-export

> **If you've written backend code…** Printed holes are like JSON numbers — `2.0` isn't exactly 2.0 after float serialization. These files already budget slack; a badly calibrated printer can still eat it.

**Screws:** assembly uses **2 mm diameter** screws, no glue. Exact lengths/counts aren't fully documented yet — use the [parts table](../../3d_models/README.md) as inventory and test fit as you go.

---

## Editing the CAD — Fusion 360

### Source vs export

| File | What it is |
| --- | --- |
| [`cad/TinyEngineer.f3d`](../../cad/TinyEngineer.f3d) | **Parametric source** — full assembly in Autodesk Fusion |
| [`3d_models/parts/*.3mf`](../../3d_models/parts/) | **Exported meshes** — what the printer eats |

> **If you've written backend code…** `.f3d` is the repo. `.3mf` is the release binary. Don't edit `.3mf` as source — edit Fusion, re-export.

### When to open Fusion

- Servo pocket too tight/loose after first print
- Different micro servo dimensions (not just HD-1370A)
- Cable routing channel for your wiring harness
- Antenna keep-out adjustment
- Desk/chair scale for your desk

Parts are designed around **HD-1370A** dimensions and **2 mm** screws. Swapping servos usually means CAD edits, not just firmware.

### Export workflow

1. Open `TinyEngineer.f3d` in Fusion (free personal license exists)
2. Edit the body you need
3. Export that body to `.3mf` in `3d_models/parts/` (or your fork)
4. Print and test fit

Alternatives (FreeCAD, Onshape) can work but project source is Fusion.

---

## Mechanical assembly principles

Full step-by-step SOP is still a doc gap — these principles keep you out of trouble:

### Servo installation

- **Orientation matters** — spline exit direction must match linkage. Check before screwing pocket closed
- **Screw horn with servo at center** — boot firmware or `/test/servo` to 90° first
- **Don't over-tighten horn screw** — strips plastic hub

### Range of motion before closing shell

Command each joint through safe range ([robot-movement.md](../robot-movement.md)). Listen for buzzing (stall). Adjust horns or firmware limits before trapping servos in plastic.

### USB and service access

Leave the USB-C breakout reachable for reflash. You'll flash again. Trust me.

### Antenna clearance

ESP32-C3-Zero ceramic antenna needs air. Don't bury it against metal or a thick plastic wall. Wi-Fi is the control plane for your agent.

### No glue policy

Screws only — disassembly for fixes. Like modular monolith vs ball of mud.

### Desk scene

Chair, desk, laptop, bell, lamp, mug — cosmetic storytelling. Print order doesn't matter much; prioritize robot body + servos for bring-up.

Part list: [3d_models/README.md](../../3d_models/README.md).

---

## Print while you wait — parallel path

Electronics can ship slow. Start printing **structural parts** (chest, head, chair) while reading [Ch. 04](04-buses-and-protocols.md) and wiring on a bench without the shell. Parallelism beats serial idle time.

---

**Next:** [Power budgets and safety](07-power-budgets-and-safety.md)

**Reference:** [3d_models/README.md](../../3d_models/README.md) · [cad/TinyEngineer.f3d](../../cad/TinyEngineer.f3d) · [robot-movement.md](../robot-movement.md)
