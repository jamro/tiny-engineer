# Shopping list

Buy this before you print or connect. Interactive cart on the site: **[Shop](https://jamro.github.io/tiny-engineer/shop/)**. Electrical spec (voltages, limits, pin notes): [hardware/components.md](hardware/components.md). Screw lengths in assembly steps: [3d_models/README.md → Screws](../3d_models/README.md#screws). Build order: [getting-started.md](getting-started.md).

## Servo size (pick first)

> **First build:** buy **five Tower Pro SG90** (or equivalent 9 g analog micros). Print [`3d_models/parts/sg90/`](../3d_models/parts/sg90/3mf/). Do **not** open Fusion.

Servo size sets the size of the **whole robot**. Buy five of **one** model, then print (or order) the matching `parts/{servo_id}/` folder.

Assembled outer size is **height × width × depth** (H×W×D), not L×W×H.

| Choice | Robot size (approx. H×W×D) | When | Print folder |
| --- | --- | --- | --- |
| **Tower Pro SG90** (recommended) | ~13 × 15 × 12 cm | Easy to find. Bigger desk — electronics are easier to fit. Best first build. Official: [Tower Pro HK](https://torqpro.com/product/sg90-analog/) | [`parts/sg90/`](../3d_models/parts/sg90/3mf/) |
| **Feetech FS0307** | ~11 × 13 × 10 cm | Compact robot. Less common than SG90 — one of two similar-size alternatives. Buy: [Pololu 3420](https://www.pololu.com/product/3420) | [`parts/fs0307/`](../3d_models/parts/fs0307/3mf/) |
| **EMAX ES9051** | ~11 × 13 × 10 cm | Compact alternative to FS0307 (similar footprint, slightly taller). Buy whichever you can source; do not mix with FS0307. Official: [EMAX store](https://shop.emaxmodel.com/products/emax-es9051-4-3g-digital-mini-servo-for-rc-model) | [`parts/es9051/`](../3d_models/parts/es9051/3mf/) |
| **PowerHD HD-1370A** | — | Backward compatibility only — skip for a new build | [`parts/hd1370a/`](../3d_models/parts/hd1370a/3mf/) |

Non-preset servo: measure it, add a preset, export parts — [parametric design](3d/parametric-design.md).

## Electronics

The default build uses the **main control board**. PCA9685, the audio amp, and USB-C are on that PCB. You plug in the ESP32, OLED, five servos, and the speaker.

Order the open-source board from any fab. Easiest path: download Gerber / BOM / positions from [Shop](https://jamro.github.io/tiny-engineer/shop/) (no KiCad), then follow [hardware/order-main-control-board.md](hardware/order-main-control-board.md). Connect it: [hardware/main-control-board.md](hardware/main-control-board.md). Reference boards from this design were manufactured and tested; your order is still a separate fab run, so review stock and orientation before you pay. A cheaper assembled batch may show up later: [discussion #50](https://github.com/jamro/tiny-engineer/discussions/50). Fab lead time is part of ordering the board.

### Default cart

| Part | Qty | Link / notes |
| --- | --- | --- |
| Tiny Engineer main control board | 1 | Open-source PCB ([CERN-OHL-S](../3d_models/LICENSE)); reference boards manufactured and tested. Fab files on [Shop](https://jamro.github.io/tiny-engineer/shop/); sources in [`hardware/boards/main-control-board/`](../hardware/boards/main-control-board/). Order: [hardware/order-main-control-board.md](hardware/order-main-control-board.md). |
| Waveshare ESP32-C3-Zero | 1 | https://docs.waveshare.com/ESP32-C3-Zero. Needs two 1×9 2.54 mm male pin headers soldered on (often in the box), or buy the ESP32-C3-Zero-M with them fitted |
| Tower Pro SG90 (or equivalent 9 g analog micro) | 5 | Official [Tower Pro HK SG90](https://torqpro.com/product/sg90-analog/) — buy ×5 of one model; compact: Feetech [Pololu FS0307](https://www.pololu.com/product/3420) or [EMAX ES9051](https://shop.emaxmodel.com/products/emax-es9051-4-3g-digital-mini-servo-for-rc-model) on [Shop](https://jamro.github.io/tiny-engineer/shop/) |
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

One buy: M2 thread-forming (self-tapping) screws for plastic (**pan-head or button-head**) plus M2 nuts. Per-step placement stays in [3d_models/README.md → Screws](../3d_models/README.md#screws).

| Item | Qty |
| --- | --- |
| [M2 fasteners (screws + nuts)](https://www.amazon.com/1080PCS-Screw-Assortment-M2x4-25mm/dp/B0GKMVNG4D) | 8× M2×4, 27× M2×8, 15× M2×16, 6× M2 nuts — [Amazon 1080-pc M2×4–25 mm kit](https://www.amazon.com/1080PCS-Screw-Assortment-M2x4-25mm/dp/B0GKMVNG4D) |

The six `BottomCover` screws are included in the M2×8 total. Their heads must be at most **4.0 mm diameter × 2.0 mm high** to fit the recessed pockets. Servo mounting screws come in the servo bags — no separate buy.

An M2 assortment covering **M2×4–M2×16** is the easy path. Nearby lengths often work; test-fit if you substitute.

### Power and cables

| Item | Qty | Notes |
| --- | --- | --- |
| **5 V / ≥2 A** USB supply **+** USB-C **data** cable | 1 each | Need both: ≥2 A for stalled servos (1 A phone charger browns out); charge-only cables fail flash and serial. |
| [Male–female jumper wires, **~10 cm**](https://www.adafruit.com/product/1953) | 1 pack (≥10 wires) | [Adafruit 1953](https://www.adafruit.com/product/1953) (6″ / 15 cm M–F). Extend OLED and servo leads when the stock cables are short — see [assembly §15](3d/assembly.md#15-connect-servos-and-oled) |

### Printed parts

Home printer: **PLA** or **PETG**, testers first — [3d_models/README.md](../3d_models/README.md). No printer: [order printed parts](3d/order-parts.md).

## Next

Return to the [getting-started](getting-started.md) checklist ([§1 Shop](getting-started.md#1-shop) done). Then print or order.
