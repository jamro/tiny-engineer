# 3D models

Printable parts and source CAD for the Tiny Engineer desk robot.

## Source CAD (edit / resize)

The full assembly — all components composed — lives in [`cad/TinyEngineer.f3d`](cad/TinyEngineer.f3d) (Autodesk Fusion). Open that file to adjust the model or adapt it to different hardware sizing (e.g. different servos). Parametric servo sizes and the Fusion add-in: [docs/3d/parametric-design.md](../docs/3d/parametric-design.md).

## Printables

Individual parts are exported under [`parts/{servo_id}/`](parts/). Match the folder to the servos you bought ([which servo](../docs/3d/parametric-design.md)). `stl/` next to `3mf/` is the same mesh in binary STL. `step/` next to those is CAD interchange (BRep STEP), not a printable mesh for a home slicer.

Two ways to get the mechanical set (same place in the build path):

- **Print yourself** — use the **`.3mf` files in `parts/sg90/3mf/`**, **`parts/fs0307/3mf/`**, or **`parts/hd1370a/3mf/`**. Ready to print in an orientation that does not need supports. **PLA** or **PETG**. Start with [Print first](#print-first).
- **No printer?** — order the aggregated sets from a third-party service: [order printed parts](../docs/3d/order-parts.md). Orders are at your own risk; fit can vary by provider and design revision.

### Print first

**Home printers only.** Print these two small testers from that folder **before** the rest of the set. Do not print the full robot until both fit. Skip this step when ordering from a service (testers alone are rarely worth the min fee + shipping).

1. **`ServoSizingTester.3mf`** — seat a real servo; do not force it. The body should slide, tabs sit, holes line up, and the shaft should have clearance. Tight or loose → sand/ream, or adjust servo CAD params ([parametric design](../docs/3d/parametric-design.md)).
2. **`ScrewSizingTest.3mf`** — M2 screw pilot-hole tolerance. Each hole is marked with its diameter. Drive an M2 screw into each hole and pick the tightest size that still cuts a thread (does not spin freely / slide through). Set Fusion user parameter `screw_thread_diameter` to that marked value, then re-export before printing structural parts. Details: [M2 screw holes](../docs/3d/parametric-design.md#m2-screw-holes).

Assembly uses **M2 screws** that thread directly into the printed PLA/PETG — no glue, no heat-set inserts. Easy to dismount and modify later. Step-by-step join order: [docs/3d/assembly.md](../docs/3d/assembly.md). Use the [screw list](#screws) and part table below, then electrical and bring-up docs.

### Screws

**Spec:** M2 thread-forming (self-tapping) screws for plastic, **pan-head or button-head** — there are no counterbores/countersinks in any part, so heads sit on the plastic surface. CAD default pilot is **2.2 mm** (Fusion user parameter `screw_thread_diameter`); if your `ScrewSizingTest` winner differs, re-export at that diameter before printing structural parts.

**Measured pilot census** of the shipped `sg90` meshes (pilot Ø ≈ 2.1 mm; counted from exported meshes, not the Fusion source — treat as a verified starting point and test-fit as you go):

| Part | M2 pilots | Pilot depth | Notes |
| --- | --- | --- | --- |
| `AiEmblem.3mf` | 1 | 4.0 mm blind | emblem → shell |
| `Bell.3mf` | 1 | 6.6 mm through | bell → desk |
| `Chest.3mf` | 0 screws (5 channels) | 14–23 mm through | deep channels are servo-wire routing, not screws |
| `Coffee.3mf` | 1 | 8.5 mm blind | pairs with `Mug` |
| `Desk.3mf` | 10 | 5.0–7.5 mm through | frame joints |
| `DeskTop.3mf` | 5 | 2.5–3.5 mm through | pilots for top-mounted items |
| `Head.3mf` | 3 | 1× 5.0 mm + 2× 12.8 mm blind | the two deep pilots are servo tab screws |
| `LampButton.3mf` | 1 | 6.1 mm through | button retention |
| `Mug.3mf` | 1 | 2.5 mm through | pairs with `Coffee` |
| `SeatLeft.3mf` / `SeatRight.3mf` | 2 each | 7.0 mm through | chair assembly |
| Neck, arms, belly, hat, lamp, laptop, desk pad, chair | 0 | — | no M2 pilots |

Only the **head** ships servo-tab pilots — the other four servos are retained by pocket fit plus the horn screw capturing each joint. Servo horn screws come **in the servo bags** (one per joint, five total); standard servo packs also include mounting screws, but only the head needs them — the rest are spares.

**Shopping list (per robot):** ~30× **M2×6** (covers every pilot ≤ 8.5 mm deep) + ~5× **M2×12** (the two 12.8 mm head pilots + spares). One M2 assortment kit (M2×4–M2×12) is the easy path.


## Parts

Same filenames in each `parts/{servo_id}/3mf/` folder (and matching `stl/` / `step/`):

| File | Role |
| --- | --- |
| `PartsSetA.3mf` | Aggregated copper-color group for [service orders](../docs/3d/order-parts.md) (home print: use individuals below) |
| `PartsSetB.3mf` | Aggregated black-color group for [service orders](../docs/3d/order-parts.md) (home print: use individuals below) |
| `Head.3mf` | Head (pitch) |
| `Neck.3mf` | Neck (yaw) |
| `ForearmLeft.3mf` | Left forearm / hand |
| `ForearmRight.3mf` | Right forearm / hand |
| `UpperArm.3mf` | Upper arm |
| `Chest.3mf` | Chest / torso |
| `Belly.3mf` | Belly |
| `Chair.3mf` | Chair |
| `SeatLeft.3mf` | Left chair seat |
| `SeatRight.3mf` | Right chair seat |
| `Desk.3mf` | Desk structure |
| `DeskTop.3mf` | Desk top surface |
| `DeskPad.3mf` | Desk pad |
| `LaptopCase.3mf` | Miniature laptop body |
| `LaptopScreen.3mf` | Laptop screen |
| `Bell.3mf` | Service bell |
| `LampBase.3mf` | Desk lamp base |
| `LampCap.3mf` | Lamp cap |
| `LampDiffuser.3mf` | Lamp diffuser |
| `LampButton.3mf` | Lamp button |
| `Mug.3mf` | Mug |
| `Coffee.3mf` | Coffee fill |
| `Hat.3mf` | Hat |
| `ServoSizingTester.3mf` | Servo pocket fit tester |
| `ScrewSizingTest.3mf` | M2 screw pilot-hole diameter tester |
| `AiEmblem.3mf` | AI emblem detail |

## Next steps

Full build path (print → wire → flash): [docs/getting-started.md](../docs/getting-started.md).

1. Mechanical assembly: [docs/3d/assembly.md](../docs/3d/assembly.md)
2. Servo axes and safe ranges: [docs/robot-movement.md](../docs/robot-movement.md)
3. Wiring and power: [docs/hardware/wiring.md](../docs/hardware/wiring.md), [docs/hardware/README.md](../docs/hardware/README.md)
4. Bring-up and failures: [docs/hardware/testing.md](../docs/hardware/testing.md)

## License

CAD source (`.f3d`) in [`cad/`](cad/) and printable parts (`.3mf`) in [`parts/`](parts/) are licensed under [CERN-OHL-S-2.0](LICENSE). See [NOTICE](NOTICE) for copyright, Source Location, and product notice requirements.

Commercial use is allowed. If you modify and distribute Products based on these designs, reciprocal provisions require sharing your Complete Source under the same license. This documentation file is software documentation and remains under the MIT License — see [LICENSING.md](../LICENSING.md).

The **Tiny Engineer** name and logo are not licensed — see [TRADEMARK.md](../TRADEMARK.md).
