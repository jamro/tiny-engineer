# Order printed parts (no 3D printer)

No printer at home? You can still get the printed set from a third-party 3D-printing service, then follow the interleaved build path: mechanical assembly continues around wiring and flashing ([assembly guide](assembly.md) is the sequencing authority; overview in [getting started](../getting-started.md)).

Orders are at your own risk. Fit and finish can vary by provider and by design revision — especially on development builds there may be bugs — and we cannot guarantee every part will match every time.

## Provider

Any FDM bureau that accepts STL or STEP can work. **[PCBWay 3D printing](https://www.pcbway.com/rapid-prototyping/3d-printing/)** is one example that was [partially tested](https://github.com/jamro/tiny-engineer/issues/9).

## Which files to upload

1. Pick the servo folder that matches the servos you bought: `3d_models/parts/<servo_id>/` ([which servo](parametric-design.md)).
2. Prefer files under **`stl/`** (widely accepted). Use **`step/`** if the provider prefers CAD. **`3mf/`** is mainly for home slicers.
3. Upload these **three** aggregates only — not the full individual-part inventory. They group pieces meant for the same material / color:

| File | Role |
| --- | --- |
| `PartsSetA.stl` (or `.step`) | Originally silk copper; customize the color if you like |
| `PartsSetB.stl` (or `.step`) | Originally black; customize the color if you like |
| `LampDiffuser.stl` (or `.step`) | White or translucent. Intentionally thin so light passes through; may fail the provider’s minimum wall-thickness check — expect to accept a wall-thickness / damage-risk disclaimer if asked |

## Settings used in the PCBWay test

Starting point (adjust if your provider’s options differ):

| Setting | Value |
| --- | --- |
| Process | 3D printing / **PLA** (PETG should work; other materials not tested here) |
| Colors | Black / Silk Copper / White |
| Infill | **20%** |
| Threads and tapped holes | **No** |
| Inserts | **No** |

## How to place the order

UI labels change; follow the outcomes, not exact button names. The flow below matches a typical PCBWay-style quote.

1. Create or sign in to an account on the provider’s **3D printing** quote page (not PCB or CNC).
2. Start a new **3D printing / FDM** quote.
3. Upload the three files from the matching `stl/` (or `step/`) folder: `PartsSetA`, `PartsSetB`, `LampDiffuser`. Confirm units are **mm** if asked.
4. For each file, set material (PLA or PETG), color (copper-ish / black / white-or-translucent), **20%** infill, no threads, no inserts.
5. For `LampDiffuser`, if the provider flags thin walls, accept the wall-thickness risk option (wording varies) so the quote can proceed.
6. Review the quote. Printing cost and shipping are separate — shipping can dominate for a single set.
7. Submit the request, wait for technical review / approval, pay, then track shipment.
8. When the parts arrive, dry-fit the servos and M2 screws before full assembly. If you notice that the holes for the screws are too loose, you can gently wrap a small piece of tape (for example, masking tape or electrical tape) around the screw, or place a tiny piece of paper inside the hole before inserting the screw. This will help the screw grip better. Make sure everything fits as expected before final assembly.

## Next steps

Back to the interleaved build path: [getting started → Print and mechanical](../getting-started.md#2-print-and-mechanical), then [assemble](assembly.md) (wire and flash mid-assembly per that guide — not after all joins). Part inventory and home-print notes: [3d_models/README.md](../../3d_models/README.md).
