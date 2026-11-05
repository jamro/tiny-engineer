# Design

## Context

See proposal.md — Why. First cut removed multi-preset/mod/ZIP/colours/explode/anims. Remaining viewer chrome still includes Electronics/Screws toggles, a ±10/25/40° range select, an inventory status string after load, pose-scale hint copy, and step nav below variable-height step body. Pipeline kept: extract → fasteners → check_steps / check_clipping / slim check_poses → `build_glb.mjs` → `models/sg90.glb`. Shared STEP helpers live in `step_geometry.py` (split out when `presets.py` was deleted).

## Goals / Non-Goals

**Goals:**
- One published model path: `web/assembly/models/sg90.glb`.
- Delete dead modules and CI branches rather than leave unused flags.
- Keep geometry-truth path (extract, fasteners, boards, clipping on steps).
- Slim pose check to joint/static coverage so CI no longer depends on animation loops.
- Minimal viewer chrome: build steps + joint sliders at fixed ±25°; hardware always shown; stable step nav.

**Non-Goals:**
- Removing fasteners, boards.py, clipping engine, or build-step UX.
- Deleting `mods/halloween/` CAD or firmware audio overlay.
- Changing Fusion export / `3d_models/parts` layout.
- Replacing Three.js or moving CSS out of `web/css/base.css` (optional later).
- Changing `DEFAULT_RANGE` / `check_poses` default away from 25° (already the default).

## Decisions

1. **Hard-delete preset/mod pipeline, not feature-flag**  
   Remove `presets.py`, `test_presets.py`, `check_contacts.mjs`, `merge_mod.mjs`, `web/assembly/mods/`, and the `build.sh` loops. Move STEP helpers fasteners need into `step_geometry.py`.  
   *Alt considered:* keep scripts for offline use — rejected; dead code would re-grow the surface.

2. **Delete `download.js` and print staging**  
   Drop `scripts/stage_print_files.mjs`, Pages stage step, `.gitignore` `web/print/` if unused, and viewer ZIP UI. Docs point at `3d_models/` / order-parts.  
   *Alt:* keep staging without UI — rejected; unused deploy work.

3. **Keep `colors.js` as defaults-only**  
   Retain `GROUP_COLORS` / `PART_COLORS` / `HARDWARE_COLORS` / `defaultColor` / `resolveColors`. Remove encode/decode, palette UI, finishes UI, storage, Copy link.  
   *Alt:* inline hex in `main.js` — worse for tests/clarity.

4. **Strip animations from `poses.js`; keep joint math**  
   Remove `ANIMATIONS`, `RING_*` hit tables used only for loops. Keep `JOINTS`, `DEFAULT_RANGE` (25), `normalizePose`, fastener followers. `check_poses.mjs` drops animation replay; keep centre/rest and a ±1 sweep.  
   *Alt:* leave ANIMATIONS unused — rejected.

5. **Ignore legacy hashes silently**  
   No redirect/toast for `#p=` / `#m=` / `#c=` / `#f=`. Stock SG90 always.  
   *Alt:* show “unsupported” banner — noise for a docs site.

6. **Docs follow code in same change**  
   Rewrite `docs/3d/assembly-viewer.md` diagram and feature list; trim AGENTS/testing/CI mentions; remove halloween viewer links from mods READMEs; drop “Print service download” claim from order-parts if it only referred to the viewer ZIP.

7. **Always-visible electronics and fasteners**  
   Drop `#asm-electronics` / `#asm-fasteners` and the visibility branches in `layout()`. Build-step `shown` / ghost rules still hide parts that are not yet in the step.  
   *Alt:* keep toggles for dense scenes — rejected per product cut.

8. **Fixed ±25° only**  
   Remove `#asm-range` select; always pass `DEFAULT_RANGE` into `joints.set`. No UI copy about firmware scale / stock 80–100°.  
   *Alt:* keep range for calibration demos — rejected.

9. **Status: loading/error only**  
   On ready, clear or use a minimal non-inventory status (empty / short “Ready”) — no “N printed · … · SG90” string.

10. **Step nav above step body**  
    In `web/assembly/index.html` / CSS, place `.asm-step-nav` under the step counter/progress (or equivalent top slot), before title/text/photo, so Back/Play/Next stay put when content height changes. Adjust `build.js` only if element order assumptions break.

## Risks / Trade-offs

- **[Risk] Users of other presets lose in-viewer verification** → Mitigation: CAD parts for other presets remain in repo; document SG90-only viewer; restore presets later as a new change if needed.
- **[Risk] Halloween builders lose viewer guide** → Mitigation: keep mod README + CAD; link to stock viewer / assembly.md instead of `#m=halloween`.
- **[Risk] `test_viewer.mjs` / `test_build_glb.mjs` heavily assert print/mod/preset paths** → Mitigation: rewrite or delete those assertions in the same PR; CI job list updated.
- **[Trade-off] Fasteners/boards/clipping stay heavy** → Accepted; separate future simplify if needed.
- **[Trade-off] No hide-electronics for screenshots** → Accepted; screenshot tools can crop.

## Migration Plan

1. Land code + docs + CI in one PR (feature branch), including viewer polish.
2. After merge, next Pages deploy publishes only `sg90.glb`; old GLBs/print tree disappear from artifact.
3. Rollback: revert the PR; pipeline and viewer restore together.

## Open Questions

None material for this scope.
