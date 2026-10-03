# Hardware (PCB)

KiCad PCB projects for Tiny Engineer. Builders: the main control board is the default electronics path — [shopping](../docs/shopping.md) and [connect guide](../docs/hardware/main-control-board.md). Breakout modules are an advanced path for extra hardware the board does not support — [wiring](../docs/hardware/wiring.md).

Boards live under `boards/<board-name>/`. Prefer the same name for the directory and the KiCad project.

| Board | Role |
| --- | --- |
| [`main-control-board`](boards/main-control-board/) | Main board: PCA9685, MAX98357A, and USB-C on the board; ESP32-C3-Zero plugs into two sockets |

Contribution rules, KiCad version, what to commit, ERC/DRC, and PR expectations: [docs/pcb.md](../docs/pcb.md). Pre-PR checklist: [docs/pcb.md#checklist](../docs/pcb.md#checklist). Critical net checklist: [docs/pcb.md#expected-netsyml](../docs/pcb.md#expected-netsyml). Local/CI checks: `python3 scripts/check_pcb.py` (KiCad 10) — [docs/testing.md](../docs/testing.md).

This tree is KiCad source only. Robot electrical reference (pinout, wiring, BOM) is [`docs/hardware/`](../docs/hardware/README.md). Firmware drivers are `src/hardware/`.

## License

Board sources in `boards/` are [CERN-OHL-S-2.0](LICENSE). See [NOTICE](NOTICE) for copyright, Source Location, and product notice requirements.

This README is software documentation and remains under the MIT License — [LICENSING.md](../LICENSING.md).

The **Tiny Engineer** name and logo are not licensed — [TRADEMARK.md](../TRADEMARK.md).
