# Licensing

This repository contains both software and hardware design files. They are licensed separately.

## Software — MIT License

The following are licensed under the [MIT License](LICENSE):

| Path | Contents |
| --- | --- |
| `src/`, `include/`, `lib/`, `test/` | Firmware |
| `packages/`, `scripts/`, `package.json` | Integrations and tooling |
| `data/`, `assets/` | Firmware filesystem assets |
| `docs/` | Documentation (including wiring diagrams and hardware guides) |
| `.cursor/` | Cursor hook configuration |
| `platformio.ini`, `partitions.csv` | Build configuration |
| All other repository files | Unless explicitly listed under hardware below |

## Hardware designs — CERN-OHL-S-2.0

Mechanical CAD, printable manufacturing outputs, and PCB sources are licensed under the [CERN Open Hardware Licence Version 2 — Strongly Reciprocal (CERN-OHL-S-2.0)](3d_models/LICENSE):

| Path | Contents |
| --- | --- |
| `3d_models/cad/` | Source CAD (`.f3d`) |
| `3d_models/parts/` | Printable part exports (`.3mf`) |
| `hardware/boards/` | KiCad PCB projects |

Commercial use of these hardware designs is allowed. If you modify and distribute Products based on these designs, the reciprocal provisions of CERN-OHL-S-2.0 require that you make the corresponding Complete Source available under the same license. See [3d_models/LICENSE](3d_models/LICENSE), [3d_models/NOTICE](3d_models/NOTICE), [hardware/LICENSE](hardware/LICENSE), and [hardware/NOTICE](hardware/NOTICE) for copyright, warranty disclaimer, and Source Location details.

Documentation in `3d_models/README.md` and `hardware/README.md` is software documentation and remains under the MIT License.

### Source Location

Canonical mechanical design source:

`https://github.com/jamro/tiny-engineer/tree/main/3d_models`

Canonical PCB design source:

`https://github.com/jamro/tiny-engineer/tree/main/hardware`

When distributing a specific revision, link to the corresponding tagged release or commit (for example, `https://github.com/jamro/tiny-engineer/tree/v0.1.0/3d_models`).

## Trademarks

The **Tiny Engineer** name and logo are **not** licensed under either MIT or CERN-OHL-S-2.0. See [TRADEMARK.md](TRADEMARK.md).

## Machine-readable license mapping

Per-path SPDX annotations are in [REUSE.toml](REUSE.toml).
