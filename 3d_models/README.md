# 3D models

Printable parts and source CAD for the Tiny Engineer desk robot.

## Source CAD (edit / resize)

The full assembly — all components composed — lives in [`cad/TinyEngineer.f3d`](cad/TinyEngineer.f3d) (Autodesk Fusion). Open that file to adjust the model or adapt it to different hardware sizing (e.g. different servos).

## Printables

Individual parts are exported to [`parts/`](parts/) as `.3mf` files. They are ready to print in an orientation that does not need supports. Print with **PLA** or **PETG**.

Assembly uses a bunch of **2 mm diameter screws** — no glue. Easy to dismount and modify later. Exact screw lengths/counts and full assembly steps are **not documented yet**. Use the part table below, then electrical and bring-up docs.

## Parts

| File | Role |
| --- | --- |
| `Head.3mf` | Head (pitch) |
| `Neck.3mf` | Neck (yaw) |
| `HandLeft.3mf` | Left hand |
| `HandRight.3mf` | Right hand |
| `Arm.3mf` | Arm (print twice) |
| `Chest.3mf` | Chest / torso |
| `Belly.3mf` | Belly |
| `Chair.3mf` | Chair |
| `Pillow.3mf` | Chair pillow |
| `Desk.3mf` | Desk structure |
| `DeskTop.3mf` | Desk top surface |
| `DeskPad.3mf` | Desk pad |
| `LaptopCase.3mf` | Miniature laptop body |
| `LaptopScreen.3mf` | Laptop screen |
| `Bell.3mf` | Service bell |
| `LampBase.3mf` | Desk lamp base |
| `LampCap.3mf` | Lamp cap |
| `LampGlass.3mf` | Lamp glass |
| `LampButton.3mf` | Lamp button |
| `Mug.3mf` | Mug |
| `MugCoffee.3mf` | Coffee fill |
| `AiEmblem.3mf` | AI emblem detail |

## Next steps

Full build path (print → wire → flash): [docs/getting-started.md](../docs/getting-started.md).

1. Servo axes and safe ranges: [docs/robot-movement.md](../docs/robot-movement.md)
2. Wiring and power: [docs/hardware/wiring.md](../docs/hardware/wiring.md), [docs/hardware/README.md](../docs/hardware/README.md)
3. Bring-up and failures: [docs/hardware/testing.md](../docs/hardware/testing.md)
