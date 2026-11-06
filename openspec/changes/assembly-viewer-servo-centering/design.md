# Design

## Context

See proposal.md — Why. Build mode already reads `web/assembly/steps.json`, flattens sections in `steps.js`, and paints title / text / screws / photo in `build.js` (`textContent` only). Mechanical truth for centering lives in `docs/3d/assembly.md` §2a and the steps that link to it (§3, §4, §7, §18). Spec already forbids runtime joint posing; this change is instructional UI + step data only.

## Goals / Non-Goals

**Goals:**

- One shared centering recipe in the UI, driven by a step flag, matching §2a.
- Early Bench window section so the rule is taught before the first shaft attach.
- Validation that keeps shaft-attach steps marked and pocket-only seats unmarked.
- Stable top nav when the callout appears or disappears.

**Non-Goals:**

- In-viewer Move all to 90° / joint animation (robot + setup AP own that).
- Changing firmware, setup wizard, or `assembly.md` mechanical procedure (viewer may link to §2a; do not fork the recipe meaning).
- Generic multi-kind notice system beyond centering (keep the field specific unless a second kind appears later).

## Decisions

### 1. Step flag: `center: true`

Add optional boolean `center` on a step in `steps.json`. When true, `build.js` shows the callout; when absent/false, hide it.

**Why over free-form `notice` text:** recipe must stay identical across steps and match §2a; duplicating long prose in every step drifts. Boolean + one UI template is the DRY path chosen in explore option 2.

**Alternatives considered:** richer `notice: { kind: "center" }` (overkill for one kind); prose-only (rejected — easy to skim past).

### 2. Shared callout copy lives in the viewer, not in each step `text`

Hardcode (or a tiny constant next to the build UI) the procedure bullets from §2a. Step `text` stays about the mechanical attach for that joint (orientation, screws, unplug). Optional short lead-in in `text` is fine; do not paste the full AP/wizard recipe into every step.

**Link:** callout can point at the section’s `docs` / `#2a-bench-window-centering` (or the step’s section doc) for the guide.

### 3. Bench window as its own section

Insert a section after `hat`, before `neck`, e.g. `id: "bench-window"`, `doc: "#2a-bench-window-centering"`, with one or two steps (`add: []`, no 3D motion). Mark those steps `center: true` so the callout appears while teaching the rule. No parts move — same empty-add pattern as the final setup step.

### 4. Which shipped steps get `center: true`

| Step (current / after split) | Mark |
| --- | --- |
| Bench window step(s) | yes |
| Center the neck servo | yes |
| Press Neck onto the shaft | no (already centered prior step) |
| Fit Neck onto the head servo | yes (centers head servo before attach) |
| Center the last servo | yes |
| Press BellyJoint onto the shaft | no |
| Mount the arms on the chest servos | yes |
| Chest side pocket seats | no |

If a center+attach is still one step after edits, that single step stays marked. Prefer keeping/extending the existing “Center …” then “Press …” splits where they already exist (neck, belly).

### 5. Validation

Extend `validate()` in `steps.js` only lightly: if `center` is present it MUST be boolean. Coverage of the four shaft-attach joints is enforced by a small explicit check in `test_viewer.mjs` (or sibling) against shipped `steps.json` titles/ids — not a fragile heuristic over all text. Document the flag in `docs/3d/assembly-viewer.md` Steps section.

### 6. Presentation

Reuse the site `.callout` amber left-border pattern as a compact `<details>` disclosure under the step title (before body text). Collapsed: one-line “Stop. Center at 90°…” plus a How affordance. Expanded (click/tap summary): full §2a recipe + guide link. Native `<details>` keeps keyboard/AT support; leave `open` sticky while stepping between center-marked steps so builders do not re-open every time. Keep Back/Play/Next in `.asm-step-nav` above changing content (existing stable-nav requirement).

### 7. Bench board visible on centering steps

`sceneAt` treats `center: true` as a view hint: preview `MainControlBoard`, `ESP32-C3-Zero`, and `ESP32-pin-headers` even before their `add` steps (separate `preview` set so clipping still uses real build `shown` only). Ghost desk/lamp covers when already shown. Merge the bench parts into `focus` so `flyTo` frames the plug-in target with the joint. No steps.json edits per center step.

## Risks / Trade-offs

- **[Risk] Callout + photo make the side panel tall on small screens** → Mitigation: keep recipe short; stable nav already scrolls below; do not put callout above nav.
- **[Risk] Recipe text drifts from assembly.md** → Mitigation: one UI string + doc link; task to compare wording to §2a when editing; no second source in steps.json.
- **[Risk] Marking the wrong steps** → Mitigation: table above + test listing expected marked step titles.
- **[Trade-off] Boolean vs generic notices** → Start boolean; promote to `notice.kind` only if a second kind appears.

## Migration Plan

Ship with Pages as ordinary static web assets. No GLB rebuild required (step JSON / JS / CSS only). Rollback = revert those files. Existing `#s=N` deep links may shift by the inserted Bench window step count — acceptable; document in PR if step indices matter to anyone.

## Open Questions

None that block specs or tasks; copy polish (exact callout heading) can be decided at implement time as long as §2a content is covered.
