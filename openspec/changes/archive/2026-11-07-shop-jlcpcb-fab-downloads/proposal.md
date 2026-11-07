# Proposal

## Why

`/shop/` currently tells builders to install KiCad 10 and Fabrication Toolkit just to obtain Gerber ZIP, `bom.csv`, and `positions.csv` for a JLCPCB both-sides PCBA order. That is the main friction on the long-pole board path. Builders should download those files from the site; stock and orientation review at JLCPCB remains manual.

## What Changes

- Generate a JLCPCB-oriented manufacturing package for `main-control-board` automatically at **GitHub Pages build time** (not committed under `hardware/**/production/`, not Release assets).
- Publish the package under the deployed site as `web/shop/fab/main-control-board/` (Gerber ZIP, `bom.csv`, `positions.csv`).
- Update the shop JLCPCB lane so the primary path is download → upload at JLCPCB → review stock/orientation — **no KiCad install** for builders.
- Keep the full order guide’s KiCad/self-export path as a secondary / advanced option; document that published files track `main` (always latest board sources at deploy).
- Trigger Pages when main-control-board KiCad sources change so the package stays fresh without tag/Release pinning.
- Prefer Fabrication Toolkit CLI (same tool documented today) for output fidelity; spike and fall back only if headless CI cannot run it.

## Capabilities

### New Capabilities

- `pcb-fab-package`: Build-time generation and publication of the main-control-board JLCPCB manufacturing package (Gerber ZIP, BOM, positions) for builders on the public site.

### Modified Capabilities

- `web-shop`: JLCPCB board lane primary steps use same-origin fab downloads instead of requiring local KiCad + Fabrication Toolkit export.

## Impact

- `.github/workflows/pages.yml` — install/export step, path triggers for `hardware/boards/main-control-board/**`, optional cache key for fab tooling.
- New script(s) under `scripts/` (or similar) to produce files into `web/shop/fab/main-control-board/` during Pages deploy.
- `web/shop/cart.json` / shop JS — download links and revised short steps.
- `docs/hardware/order-main-control-board.md` (and brief shop-related notes) — download-first flow; KiCad export remains available.
- `hardware/**/production/` stays gitignored; no change to contributor “do not commit Gerbers” baseline for source PRs.
- Out of scope: GitHub Release / tag-pinned fab packages; automating JLCPCB checkout or stock substitution; other boards; selling assembled boards.
