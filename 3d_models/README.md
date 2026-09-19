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

Assembly uses **M2 screws** that thread directly into the printed PLA/PETG — no glue, no heat-set inserts. Easy to dismount and modify later. Step-by-step join order: [docs/3d/assembly.md](../docs/3d/assembly.md). Use the [screw list](#screws) below, then electrical and bring-up docs.

### Screws

**Spec:** M2 thread-forming (self-tapping) screws for plastic, **pan-head or button-head** — there are no counterbores/countersinks in any part, so heads sit on the plastic surface. CAD default pilot is **2.1 mm** (Fusion user parameter `screw_thread_diameter`); if your `ScrewSizingTest` winner differs, re-export at that diameter before printing structural parts. Details: [M2 screw holes](../docs/3d/parametric-design.md#m2-screw-holes).

**One BOM for every servo preset** (`parts/sg90/`, `parts/fs0307/`, `parts/hd1370a/`, …). Printed parts scale with the servo choice, but the lengths below are the reference shopping list for all of them — including smaller models for smaller servos. Pilots are cut as deep as practical so nearby lengths often work too; if you already have different M2 lengths, test-fit before buying a full set. Per-step placement: [assembly guide](../docs/3d/assembly.md).

**Shopping list (per robot):**

| Item | Qty | Typical use |
| --- | --- | --- |
| M2×4 mm | 8 | Emblem, PCA9685, MAX98357A |
| M2×8 mm | 21 | Hat, Neck↔Head, Chest center (prefer two), Belly↔Chest, LampBase, laptop, mug, Chair↔Desk, elbows |
| M2×16 mm | 15 | Chest side servos, SeatLeft/Right, DeskTop stack, USB-C, Bell |
| M2 nuts | 6 | Neck captured ×1, USB-C ×4, Bell ×1 |
| Servo bag screws | 5 | Neck shaft, Neck↔Head (robot’s left), Belly shaft, both arms |

Prefer **two** M2×8 for the Neck servo in `Chest`; one at the back is enough if space is tight (buy 21 either way). An M2 assortment covering **M2×4–M2×16** is the easy path.


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
