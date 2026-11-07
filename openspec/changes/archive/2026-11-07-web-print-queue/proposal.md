# Proposal

## Why

`/print/` is a stub that dumps builders into GitHub `3d_models/README.md`. After Shop they already picked a servo, but Print does not show the matching part list, color groups, downloadable meshes, or print progress. The build timeline treats Print as stage 2; it should make home printing as guided as Shop — without becoming a print bureau storefront.

## What Changes

- Replace the `/print/` stub with a print-queue page driven by same-origin `parts.json`.
- Read the Shop servo preset from `localStorage` (`tiny-engineer.shop.servo`); show the variant as read-only and send changes back to `/shop/` (no servo picker on Print).
- List printable parts grouped by design color (PartsSetA copper, PartsSetB black, LampDiffuser white/translucent), plus a separate **Print first** tester section with its own checkboxes.
- Persist print progress checkboxes in `localStorage` (separate from Shop checks).
- Offer per-part same-origin downloads for both `.3mf` and `.stl`.
- Show a **live** WebGL preview of each listed part from its same-origin `.stl` (not static PNGs).
- Publish individual meshes under `web/print/files/{servo}/` at Pages deploy time (CI copy from `3d_models/parts/…`); do not commit the mesh bulk into git.
- Visual language matches Shop / `web/STYLE.md` (panels, tokens, progress, continue CTA).
- **Non-goals:** print-service order wizard (PCBWay-style aggregates), HD-1370A on the Print UI, kit pricing, Fusion/CAD editing, committing ~47 MB of meshes to the repo, static preview PNG pipeline.

## Capabilities

### New Capabilities

- `web-print`: Public GitHub Pages print stage — servo-locked part queue, color-grouped checklist, live STL previews, dual-format downloads, and progress persistence that help builders finish the “printed set ready” exit criterion before connect.

### Modified Capabilities

- (none) — `web-shop` already requires navigating to `/print/` and forbids hosting the print wizard on Shop.

## Impact

- `web/print/` (HTML + `parts.json` + JS)
- New print UI script(s) under `web/js/` (Three.js / STLLoader, following assembly import-map pattern)
- CSS in `web/css/base.css` / tokens as needed — no new visual system
- `.github/workflows/pages.yml`: copy `3mf`/`stl` individuals into `web/print/files/{sg90,fs0307,es9051}/` before Pages upload
- Docs: optional one-line pointers from `3d_models/README.md` / getting-started to `/print/`; keep part/color truth aligned with assembly print groups
- No firmware, HTTP API, or package changes
