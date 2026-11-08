# Proposal

## Why

Builders open `/flash/` and hit Connect & install before they know the ESP32 must sit on the main control board and that USB goes into the **board**, not the module. Wrong orientation or a bare module still “flashes,” then fails Wi‑Fi / PCA9685 checks. The site already ships the assembly GLB with those parts — flash should use it as a short visual prep before the installer.

## What Changes

- Add a **Prepare the board** section at the **top** of `/flash/`, before the install / image-map panels.
- Two-step guided 3D walkthrough (Prev / Next) using the published SG90 assembly model (`web/assembly/models/sg90.glb`), showing only bench electronics (`MainControlBoard`, `ESP32-C3-Zero`, `ESP32-pin-headers`) — not the assembled desk.
- Step 1: seat the module into the board sockets with orientation called out via the PCB USB mark; animate the seat from above.
- Step 2: highlight that the USB cable plugs into the **board** USB-C and must not use the small port on the ESP32 module.
- Keep the existing release picker, esp-web-tools install button, and image-map panel below the prep section.
- Beginner-plain copy for prep titles and body (aligned with build-guide voice); demote or shorten duplicate callouts in the install panel foot that only restate seating / USB.

## Capabilities

### New Capabilities

- `web-flash`: Public GitHub Pages flash stage — board preparation guidance and browser USB firmware install for the stock Tiny Engineer main control board path.

### Modified Capabilities

- (none) — reuses the published assembly GLB as a shared asset; does not change `assembly-viewer` requirements. `web-build-guide` is not yet archived to main specs; prep copy still follows that beginner-voice intent without a delta here.

## Impact

- `web/flash/index.html` — new prep layout above `.hero`.
- New thin module under `web/js/` (e.g. `flash-prep.js`) reusing `web/js/assembly/stage.js` + Three/GLTF from the same CDN import map pattern as `/assembly/`.
- CSS in `web/css/base.css` (and tokens if needed) for prep stage + step card.
- Loads `../assembly/models/sg90.glb` (same-origin on Pages); no new GLB build pipeline.
- Docs: optional light cross-link from `docs/flash.md` / getting-started to the web prep; no requirement to rewrite Markdown tone.
- No firmware, HTTP API, or NVS changes.
