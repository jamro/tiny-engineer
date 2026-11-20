# Proposal

## Why

Users can pick classic, kaomoji, cover, dots, or kawaii OLED faces, but nothing matches a sharp, always-menacing “evil eyes” look. A sixth `eyes_style` lets the desk robot wear that persona without new animation names or servo/audio changes.

## What Changes

- Add `eyes_style` value **`evil`**: solid white scowling silhouettes (pointed high outers, rounded low inners, concave brow notch, convex bottoms; no pupils), mirrored left/right on the 128×32 OLED
- Reuse the classic eye pose pipeline for blinks, glances, height, sleep close/open, and dead X — same motion language as `dots`, different paint
- Always look evil for every `EyeMode` / `POST /anim` (no cute moods, no kaomoji face table)
- Wire the id through settings validate, style registry, Config UI, API docs, and (optionally) the public `/eyes` preview gallery
- Default stays `classic`; existing saved styles unchanged

## Capabilities

### New Capabilities

- `evil-eyes`: OLED face style `evil` — registration, look, and how it tracks classic pose / blink / sleep / dead

### Modified Capabilities

- (none)

## Impact

- Firmware: `src/display/eyes/styles/` (new renderer + registry / `eye_style_ids`), settings validate already keyed off registered ids
- HTTP/settings: `eyes_style` accepts `evil`; GET/POST unchanged otherwise
- Web UI Config select; `docs/api.md` + `ui` ApiReference param lists
- Marketing gallery `web/eyes/` + preview JS (same pattern as dots/classic)
- Tests: `pio test -e native` eyes-style validate; `pio run` after style code
- No new `/anim` names, packages, NVS key rename, or flash-size asset pack (procedural draw preferred)
