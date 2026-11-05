# Proposal

## Why

The 3D assembly viewer and its CI pipeline grew into a multi-preset, multi-mod, share-and-download product (~7k LOC) while the core need is a single stock SG90 robot: build steps, default colours, and manual joint poses. Cutting unused surface area shrinks maintenance, CI time, and Pages deploy complexity without dropping the CAD→GLB truth path. After the first cut, remaining chrome (visibility toggles, range picker, inventory status, moving step nav) still distracts from that core.

## What Changes

- **BREAKING (viewer):** Drop servo preset switch; only SG90. Hash `#p=` and non-SG90 GLBs go away.
- **BREAKING (viewer):** Remove Halloween (and any) mod from the web assembly viewer. Hash `#m=` and `web/assembly/mods/` configs go away. `mods/halloween/` CAD/firmware overlay stays; only viewer integration is removed.
- **BREAKING (viewer):** Remove print-file ZIP download and Pages staging of `web/print/`.
- **BREAKING (viewer):** Remove colour picking, colour share links (`#c=`), fastener finish UI, and Copy link colour persistence.
- **BREAKING (viewer):** Remove explode slider.
- **BREAKING (viewer):** Remove animation play loops (typing/talking/ring/wave). Keep per-joint pose sliders.
- **BREAKING (viewer):** Remove Electronics and Screws visibility toggles; electronics and fasteners stay always visible.
- **BREAKING (viewer):** Remove the servo angular-range control; joints always use ±25°.
- Remove the post-load inventory status line (printed / electronics / screws counts · SG90). Keep short loading and error status only.
- Remove the joint pose-scale hint copy under the sliders.
- Pin build-step Back / Play / Next to the upper part of the build card so the controls do not jump when step text length changes.
- Pipeline: stop building other servo presets and mods; delete or stop calling `presets.py`, `merge_mod.mjs`, `check_contacts.mjs`, `stage_print_files.mjs`, and related tests.
- Slim `check_poses` to joint-range / static pose checks only (no animation loop replay).
- Docs (`docs/3d/assembly-viewer.md` and cross-links) match the reduced surface.

**Out of scope (kept):** fasteners auto-placement, boards/ESP32 meshes, clipping checks for build steps, build-step UI (with stable nav), default (static) part colours, extract→GLB for SG90, Reset view, `#s=` deep links, 90° centre button.

## Capabilities

### New Capabilities

- `assembly-viewer`: Browser 3D assembly model for the stock SG90 robot — build steps, default colours, joint sliders at fixed ±25°, and the SG90-only CI build/check pipeline that feeds it.

### Modified Capabilities

- (none — no existing OpenSpec capability covers the assembly viewer)

## Impact

- **Web:** `web/assembly/`, `web/js/assembly/` (especially `main.js`, `poses.js`, `build.js` / CSS for step nav; delete `download.js`; slim `colors.js`), shared nav unchanged except docs links.
- **Scripts:** `scripts/assembly/build.sh` and callers; remove/stop preset/mod/print paths; CI job `assembly` and Pages workflow.
- **Docs:** `docs/3d/assembly-viewer.md`, `docs/testing.md`, `AGENTS.md`, `mods/README.md`, `mods/halloween/README.md`, shopping/order-parts mentions of viewer download.
- **Users:** Bookmarks with `#p=`, `#m=`, `#c=`, or `#f=` stop applying. No in-viewer print ZIP — use `3d_models/` / order-parts docs instead. No range or visibility toggles in the viewer chrome.
