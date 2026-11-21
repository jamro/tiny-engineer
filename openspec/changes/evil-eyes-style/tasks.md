# Tasks

## 1. Firmware style + registration

- [x] 1.1 Add `src/display/eyes/styles/evil.cpp` implementing `kEvilEyeStyle` (`id` `evil`, pose+blink on) that paints mirrored solid white scowling silhouettes (pointed high outers, rounded low inners, concave brow notch, convex bottoms; no pupils) from classic `renderEye`/`blinkOpenAmount`, skips fill when height is closed, and uses `drawDeadXEyes` on the dead X hold — verify `pio run` links the new object
- [x] 1.2 Register `kEvilEyeStyle` in `registry.cpp` and add `"evil"` to `eye_style_ids.cpp`; extend `test_eyes_style` in `test/test_settings_validate` to accept `evil` and still reject unknowns — verify `pio test -e native` passes
- [x] 1.3 Note `evil` in `src/display/eyes/styles/README.md` (table + add-a-style list stays accurate) — verify the row describes the scowling silhouette look

## 2. Config, API docs, UI

- [x] 2.1 Add `evil` to Config `EYES_STYLES` and refresh the hint so it mentions the new style — verify `npm run lint --prefix ui` passes
- [x] 2.2 Document `evil` in `docs/api.md` (`eyes_style` field, POST allowed values, short “Evil eyes” section parallel to Cover/Dots) and in `ui/src/views/ApiReference.tsx` param text — verify the listed values match exactly across both

## 3. Public eyes gallery

- [x] 3.1 Add an `evil` card on `web/eyes/index.html` and a canvas preview in `web/js/eyes-preview.js` that approximates the scowling silhouette (blink/pose like dots) — verify the page lists six previewable styles including `evil` (cover may remain footnote-only)

## 4. Integration

- [x] 4.1 Run `pio run` after the firmware/docs/UI edits and verify the default env builds clean

## Workflow follow-up

- Archive the change after review when implementation is done (`/opsx-archive` or project equivalent)
- On-device visual check of open, blink, glance, sleep, and dead X with `eyes_style=evil` if hardware is available
