# 3D models

Printable parts and source CAD for the Tiny Engineer desk robot.

## Source CAD (edit / resize)

The full assembly — all components composed — lives in [`cad/TinyEngineer.f3d`](cad/TinyEngineer.f3d) (Autodesk Fusion). Open that file to adjust the model or adapt it to different hardware sizing (e.g. different servos). Parametric servo sizes and the Fusion add-in: [docs/3d/parametric-design.md](../docs/3d/parametric-design.md).

## Printables

Individual parts are exported under [`parts/{servo_id}/`](parts/). Print the **`.3mf` files in `parts/sg90/3mf/`**, **`parts/fs0307/3mf/`**, or **`parts/hd1370a/3mf/`** to match the servos you bought ([which servo](../docs/3d/parametric-design.md)). They are ready to print in an orientation that does not need supports. Print with **PLA** or **PETG**. `stl/` next to `3mf/` is the same mesh in binary STL. `step/` next to those is CAD interchange (BRep STEP), not a printable mesh.

Assembly uses a bunch of **2 mm diameter screws** — no glue. Easy to dismount and modify later. Exact screw lengths/counts and full assembly steps are **not documented yet**. Use the part table below, then electrical and bring-up docs.

## Parts

Same filenames in each `parts/{servo_id}/3mf/` folder:

| File | Role |
| --- | --- |
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
| `ServoSizingTester.3mf` | Servo pocket fit tester (print first when changing servo size) |
| `AiEmblem.3mf` | AI emblem detail |

## Next steps

Full build path (print → wire → flash): [docs/getting-started.md](../docs/getting-started.md).

1. Servo axes and safe ranges: [docs/robot-movement.md](../docs/robot-movement.md)
2. Wiring and power: [docs/hardware/wiring.md](../docs/hardware/wiring.md), [docs/hardware/README.md](../docs/hardware/README.md)
3. Bring-up and failures: [docs/hardware/testing.md](../docs/hardware/testing.md)

## License

CAD source (`.f3d`) in [`cad/`](cad/) and printable parts (`.3mf`) in [`parts/`](parts/) are licensed under [CERN-OHL-S-2.0](LICENSE). See [NOTICE](NOTICE) for copyright, Source Location, and product notice requirements.

Commercial use is allowed. If you modify and distribute Products based on these designs, reciprocal provisions require sharing your Complete Source under the same license. This documentation file is software documentation and remains under the MIT License — see [LICENSING.md](../LICENSING.md).

The **Tiny Engineer** name and logo are not licensed — see [TRADEMARK.md](../TRADEMARK.md).
