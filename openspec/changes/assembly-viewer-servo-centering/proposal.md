# Proposal

## Why

Builders using the web assembly viewer can miss the hard stop to electrically center a servo before pressing a printed part onto its shaft. Today that guidance is buried in step body text (and sometimes incomplete), so horns and joints go on at the wrong angle and later calibration fights the mistake. The mechanical guide already has a clear recipe in `docs/3d/assembly.md` §2a; the viewer should surface the same rule whenever a step needs it.

## What Changes

- Add a structured centering notice on build steps that require shaft centering before attaching a part (Neck, Head↔Neck join, BellyJoint, arms).
- Render that notice as a distinct callout in the build card with the shared §2a procedure (setup AP → Assembly support → Move all to 90°, optional Identify, unplug after), not as buried prose alone.
- Introduce an early “Bench window” section aligned with assembly.md §2a so the recipe is taught once before the first centering attach.
- Keep the fixed assembled pose: no in-viewer joint posing or 90° animation (existing requirement unchanged).
- Update step validation / viewer tests so centering-marked steps stay in sync with the guide.

## Capabilities

### New Capabilities

<!-- none -->

### Modified Capabilities

- `assembly-viewer`: require a clear, shared centering callout (and matching step data) whenever a build step needs servos centered before attaching a part; include the §2a bench-window section in the step sequence.

## Impact

- `web/assembly/steps.json` — mark centering steps; add §2a section; tighten step texts so they do not duplicate the full recipe awkwardly
- `web/js/assembly/build.js`, `web/assembly/index.html`, `web/css/base.css` — render/hide the callout; stable nav still applies
- `web/js/assembly/steps.js` — optional validation for the centering flag
- `scripts/assembly/test_viewer.mjs` (and related docs if the steps schema is described) — cover the new field and callout behavior
- Docs: `docs/3d/assembly-viewer.md` step-schema note only; mechanical procedure stays owned by `docs/3d/assembly.md`
