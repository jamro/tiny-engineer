# Order the main control board

Build the integrated PCB yourself from the open-source KiCad design, then return to the [getting-started](../getting-started.md) checklist. After the boards arrive, connect modules with [main-control-board.md](main-control-board.md).

**Any PCB manufacturer can fabricate this board.** Upload formats, BOM / pick-and-place conventions, assembly options, and part stock differ by fab. If you use someone other than the provider below, adapt the steps to their process and always re-check the bill of materials and part availability before you pay for assembly.

This walkthrough uses **[JLCPCB](https://jlcpcb.com/)** as one common place to order PCB + assembly (PCBA) with LCSC parts. Tiny Engineer has **no partnership** with JLCPCB — the project documents one provider the author chose; you are free to use another.

Orders are at your own risk. Fabrication and assembly can go wrong (missing parts, wrong orientation, stock substitutions). Design revisions — especially early boards — can have bugs. The project cannot guarantee every order yields a working board.

## What you still buy separately

The fab builds the PCB and (if you choose PCBA) the parts soldered onto it. You still need the plug-in modules and the rest of the robot cart: [shopping.md](../shopping.md).

## Prerequisites

- **KiCad 10** (project major — see [pcb.md](../pcb.md))
- KiCad **Fabrication Toolkit** plugin (Plugin and Content Manager → search for Fabrication Toolkit). It exports Gerbers plus JLCPCB-oriented BOM and position files.
- An account at your chosen fab (here: JLCPCB)

## 1. Export manufacturing files from KiCad

1. Open [`hardware/boards/main-control-board/main-control-board.kicad_pro`](../../hardware/boards/main-control-board/main-control-board.kicad_pro).
2. Run **Fabrication Toolkit** so it writes manufacturing outputs under the board folder (typically a `production/` directory).
3. Confirm you have at least:
   - a Gerber **ZIP** (board layers + drills)
   - **`bom.csv`** (bill of materials)
   - **`positions.csv`** (component placement / CPL / pick-and-place)
4. Keep `production/` local — it is gitignored and should not be committed.

UI names for the plugin and export dialog change over time; what matters is those three artifacts.

## 2. Choose how much assembly you want

| Option | Cost / effort | This guide |
| --- | --- | --- |
| **PCB only** | Cheapest. You solder every SMD part yourself. | **Not recommended** for a first order — small SMD work is hard. **Skipped** below. |
| **PCBA, one side (top)** | Good cost compromise. Most electronics are on the front; you solder the ESP32 sockets yourself. | Covered |
| **PCBA, both sides** | Most expensive, most convenient — no soldering needed for the board parts. | Covered |

On **one-side** PCBA, the remaining hand work is soldering **2.54 mm female header** sockets for the Waveshare ESP32-C3-Zero (two 1×9 rows on this design). On **both-sides** PCBA, those headers are assembled for you.

## 3. Place the order (JLCPCB example)

Site menus and labels change. Follow the **outcomes**, not exact button names.

1. Create or sign in to a JLCPCB account and start a new **PCB** order.
2. Upload the Gerber **ZIP** from Fabrication Toolkit.
3. Keep most board options at sensible defaults. As of writing, a typical match for this board is:

   | Setting | Typical choice |
   | --- | --- |
   | Base material | FR-4 |
   | Layers | 2 |
   | Thickness | 1.6 mm |
   | Quantity | Fab minimum if you only need one board (often **5** today) |
   | PCB color | Your choice — colors other than green can cost more |
   | Surface finish | LeadFree HASL (author preference; other finishes are fine if the fab offers them) |

4. **Turn on PCB Assembly.** Leave stencil **off** for this PCBA flow — you do not need a separate stencil when the fab assembles the board.
5. Set assembly options for your path:

   **One side (you solder ESP32 headers later)**

   - PCBA type: **Economic**
   - Assembly side: **Top**
   - Assembly quantity: the fab may let you assemble **fewer** boards than the PCB quantity (for example, assemble 2 of 5). Bare leftovers arrive without parts. Assembling every board costs more but gives working spares; fewer assemblies is a valid way to cut cost.

   **Both sides (fully assembled)**

   - PCBA type: **Standard**
   - Assembly side: **Both sides**
   - For this process the fab may add **edge rails**. Enabling a **depaneling** service is often worth it so you get usable boards without cutting rails yourself.
   - Same note on assembly quantity vs PCB quantity as above.

6. Continue and **review the board layout** in the fab’s viewer. Confirm outline, holes, and copper look right, then proceed.
7. Upload **`bom.csv`** and **`positions.csv`** (the CPL / pick-and-place file), then continue.
8. **Review the bill of materials.** Stock and catalog entries change. Check every line. ESP32 header parts may warn that **multiple catalog items match** — pick the correct one manually. Resolve out-of-stock or wrong parts here (or update the schematic LCSC fields, re-export, and upload again) before you pay.
9. **Review component placement and orientation**, then continue.
10. **Review the cost quote**, complete checkout, and track the shipment.

## 4. When the boards arrive

1. Inspect for missing parts, tombstoned components, or obvious orientation mistakes.
2. **One-side PCBA:** solder the 2.54 mm female headers for the ESP32, then follow [main-control-board.md](main-control-board.md).
3. **Both-sides PCBA:** plug in the ESP32, OLED, servos, and speaker per [main-control-board.md](main-control-board.md) — no board soldering required for those sockets.
4. Return to [getting-started](../getting-started.md) for flash and mechanical assembly.

KiCad sources and electrical notes for contributors: [`hardware/boards/main-control-board/`](../../hardware/boards/main-control-board/). Contribution rules: [pcb.md](../pcb.md). Optional cheaper community batch: [discussion #50](https://github.com/jamro/tiny-engineer/discussions/50).
