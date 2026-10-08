# Shopping list

Buy this before you print or connect. Electrical spec (voltages, limits, pin notes): [hardware/components.md](hardware/components.md). Screw lengths in assembly steps: [3d_models/README.md → Screws](../3d_models/README.md#screws). Build order: [getting-started.md](getting-started.md).

## Servo size (pick first)

> **First build:** buy **five Tower Pro SG90** (or equivalent 9 g analog micros). Print [`3d_models/parts/sg90/`](../3d_models/parts/sg90/3mf/). Do **not** open Fusion.

Servo size sets the size of the **whole robot**. Buy five of **one** model, then print (or order) the matching `parts/{servo_id}/` folder.

| Choice | When | Print folder |
| --- | --- | --- |
| **Tower Pro SG90** (recommended) | Default. Bigger desk — electronics are easier to fit | [`parts/sg90/`](../3d_models/parts/sg90/3mf/) |
| **Feetech FS0307** | Compact robot | [`parts/fs0307/`](../3d_models/parts/fs0307/3mf/) |
| **EMAX ES9051** | Compact — similar footprint to FS0307, slightly taller | [`parts/es9051/`](../3d_models/parts/es9051/3mf/) |
| **PowerHD HD-1370A** | Backward compatibility only — skip for a new build | [`parts/hd1370a/`](../3d_models/parts/hd1370a/3mf/) |

Non-preset servo: measure it, add a preset, export parts — [parametric design](3d/parametric-design.md).

## Electronics

The default build uses the **main control board**. PCA9685, the audio amp, and USB-C are on that PCB. You plug in the ESP32, OLED, five servos, and the speaker.

Order the open-source board from any fab: [hardware/order-main-control-board.md](hardware/order-main-control-board.md). Connect it: [hardware/main-control-board.md](hardware/main-control-board.md). A cheaper assembled batch may show up later: [discussion #50](https://github.com/jamro/tiny-engineer/discussions/50). Fab lead time is part of ordering the board.

### Default cart

| Part | Qty | Link / notes |
| --- | --- | --- |
| Tiny Engineer main control board | 1 | KiCad in [`hardware/boards/main-control-board/`](../hardware/boards/main-control-board/) ([CERN-OHL-S](../3d_models/LICENSE)). Order: [hardware/order-main-control-board.md](hardware/order-main-control-board.md). |
| Waveshare ESP32-C3-Zero | 1 | https://docs.waveshare.com/ESP32-C3-Zero |
| Tower Pro SG90 (or equivalent 9 g analog micro) | 5 | Widely available hobby servo — no single canonical SKU |
| [Adafruit Mini Oval Speaker - 8 Ohm 1 Watt](https://www.adafruit.com/product/3923) | 1 | https://www.adafruit.com/product/3923 |
| [Waveshare 0.91inch OLED Module](https://www.waveshare.com/0.91inch-oled-module.htm) (SSD1306, 128×32, I2C) | 1 | https://www.waveshare.com/0.91inch-oled-module.htm |

Compact build: swap the five SG90 for **Feetech FS0307** or **EMAX ES9051** (same qty). Do not mix models.

The board already includes the PCA9685, the MAX98357A, and USB-C.

### Advanced: extra modules

Use separate breakouts when you want modules the main control board does not support. Same firmware and printed Desk. More soldering and more boards in the cavity. Wiring: [hardware/wiring.md](hardware/wiring.md). Desk mounting: [3d/assembly-modular.md](3d/assembly-modular.md).

| Part | Qty | Link |
| --- | --- | --- |
| Adafruit PCA9685 16-channel PWM servo driver | 1 | https://www.adafruit.com/product/815 |
| MAX98357A I2S class-D mono amplifier | 1 | https://www.aliexpress.us/item/3256805196806369.html |
| Adafruit 5993 USB-C breakout | 1 | https://www.adafruit.com/product/5993 |

These three replace the main control board. You still buy the ESP32, servos, speaker, and OLED from the default cart.

### Fasteners

M2 thread-forming (self-tapping) screws for plastic, **pan-head or button-head**. Qty below is the shopping list; per-step placement stays in [3d_models/README.md → Screws](../3d_models/README.md#screws).

| Item | Qty |
| --- | --- |
| M2×4 mm | 8 |
| M2×8 mm | 27 |
| M2×16 mm | 15 |
| M2 nuts | 6 |
| Servo bag screws | 5 (come with the servos) |

The six `BottomCover` screws are included in the M2×8 total. Their heads must be at most **4.0 mm diameter × 2.0 mm high** to fit the recessed pockets.

An M2 assortment covering **M2×4–M2×16** is the easy path. Nearby lengths often work; test-fit if you substitute.

### Power and cables

| Item | Qty | Notes |
| --- | --- | --- |
| **5 V / ≥2 A** USB supply (or a host that can actually deliver that) | 1 | Five stalled servos plus Wi-Fi and audio need headroom. A 1 A phone charger will brown out. |
| USB-C **data** cable | 1 | Charge-only cables fail flash and serial. |

### Printed parts

Home printer: **PLA** or **PETG**, testers first — [3d_models/README.md](../3d_models/README.md). No printer: [order printed parts](3d/order-parts.md).

## Next

Return to the [getting-started](getting-started.md) checklist ([§1 Shop](getting-started.md#1-shop) done). Then print or order.
