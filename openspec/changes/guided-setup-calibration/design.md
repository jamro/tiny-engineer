# Design

## Context

See proposal.md — Why. Today wizard step 1 forces `PlaceParts` (horns) then `FindRanges` (joint tabs). State lives in `useCalibration` (`phase: horns | ranges`, `joint`, `angles`, `ranges`). Moves go through `useSetupServo` → `POST /setup/servo` (~25°/s). Save on Next posts `servo_mins` / `servo_maxs` from in-memory ranges. No API or NVS shape change.

## Goals / Non-Goals

**Goals:**

- Hub-based entry replacing forced horns → tabs
- Stage A (assembly) and Stage B (guided ranges) as separate UI modes with clear return/skip rules
- Derive mirrored max in the client from a session-only center; demonstrate ranges with sequential setup moves
- Stock empty-NVS defaults of 80°–100° on every joint (safe narrow band around electrical 90°)
- Clear hub hierarchy, readable bolded instructions, and guided progress / confirm / finish affordances
- Keep Vitest coverage for hub, skip, Stage A identify, Stage B order/symmetry/hands polarity, and save payloads

**Non-Goals:**

- New HTTP routes or altering `/setup/servo` / settings validation shape
- Persisting per-joint center angles (NVS, settings fields, or browser storage beyond the live UI session)
- Re-calibrating from the STA Config page
- CLI / serial parity for setup calibration
- Migrating devices that already have wider ranges in NVS (they keep saved values; factory reset that preserves calibration also keeps them)

## Decisions

### 1. Three-mode state instead of horns/ranges only

Replace `phase: "horns" | "ranges"` with something like `mode: "hub" | "assembly" | "guided"` (names flexible). Hub is default on entering the servos step. Assembly and guided are exclusive; neither nests inside the other.

**Alternatives:** Keep forced Stage A before B (rejected — hub + skip). Separate wizard steps for A and B (rejected — clutters the 5-step progress and skip would be awkward).

### 2. Guided path as an ordered substep machine

Model Stage B as a fixed list of substeps (center / set-min / demo for body·neck·head; four hand bound steps). Advance only on explicit confirm; Back decrements index or returns to hub at 0. Drop joint tabs during guided mode.

Suggested substep ids (implementation may rename):

```
body-center → body-min → body-demo
neck-center → neck-min → neck-demo
head-center → head-min → head-demo
hand-right-min → hand-left-max → hand-left-min → hand-right-max
```

Center and min/hand substeps share the same ±1/5/10 nudge pad and angle readout. Entering a step parks at the value already held for that role in `ranges` (NVS or defaults, updated as earlier guided confirms write): `*-center` → `(min + max) / 2`; `*-min` / hand `bound: min` → current `min`; hand `bound: max` → current `max`. Same pattern as center: show the saved bound, then nudge. On confirm of a `*-center` step: store `centers[joint] = angle` in calibration hook state only. On confirm of a `*-min` step: reject if `min >= centers[joint]`; else `max = 2 * centers[joint] - min`. Demo moves to max then back to `centers[joint]`, then auto-advance when moves succeed. Electrical 90° remains Stage A (horn seating) only.

**Alternatives:** Free tabs with a “recommended order” checklist (rejected — too easy to skip landmarks). Compute max only at save time without demo (rejected — user asked for visual sweep). Fixed electrical 90° as default guided center (rejected — stock/saved ranges already define a mid). Offset parks (`center − 20`, electrical mid ±20) for min/max (rejected — user wants open-at-saved-bound, parallel to center → mid).

### 3. Session-only center on the client (not NVS)

Keep `centers` as in-memory state in `useCalibration` for the guided path. Discard when leaving guided mode or finishing save. Runtime already treats pose `0` as the midpoint of saved min/max, so a persisted center would be redundant and wrong if it drifted from `(min+max)/2`.

**Alternatives:** NVS/settings center field (rejected — unused after save, API churn). `sessionStorage` (unnecessary; hook state lives for the wizard session). Derive center only as `(min+max)/2` without a center step (rejected — user must aim the landmark before choosing min).

### 4. Reuse existing move helper; no new endpoints

Assembly identify: for index `i`, chain `90 → 93 → 87 → 90` (or equivalent ±3) via `useSetupServo`. Guided nudges and demos: same helper. Disable controls while a move is in flight (existing `perform` pending already surfaces status).

Stage A Identify gate: keep Identify controls disabled until a successful move-all-to-90° in the current assembly visit sets a session flag (e.g. `centered`). Failed or incomplete center-all leaves Identify disabled. Leaving assembly clears the flag so the next visit requires center-all again. Rationale: the first identify hop is 93°; without a prior center, an uncentered joint can take a large jump.

**Alternatives:** Client-side interpolated PWM (impossible without firmware). Faster `/test/servo` (wrong clamp + wrong mode). Always-enabled Identify (rejected — unsafe large jump before center-all).

### 5. Wizard footer ownership

Hub: no Next into Screen until Skip or finishing guided (Skip acts as advance). Assembly: Back/Done → hub; hide the main wizard Next (same idea as today’s `placingParts` footer hide). Guided: Back = substep back; primary = Confirm / Next substep; final confirm triggers existing servos `save()` then `stepIndex + 1`.

Skip from hub: call the same save path with current `ranges` (loaded from settings), then advance — matches “mechanism unchanged.”

### 6. File split

Prefer small Preact files already used in `ui/src/setup/`:

- Hub component (new)
- Adapt `PlaceParts` → assembly mode (+ per-servo Identify)
- Replace or heavily slim `FindRanges` → guided substep view + copy
- Expand `useCalibration` (or split `useGuidedCalibration`) for mode + substep index + session `centers` + range updates

Keep `SERVO_NAMES` / index mapping; guided order uses body=4, neck=1, head=0, hands 3 then 2 as specified.

### 7. Guided hand bounds from bench (UI swap)

On-robot testing showed the docs-aligned binding was flipped for landmarks. Guided UI writes: keyboard → right **min**, left **max**; upper → left **min**, right **max**. Keep physical step order; only which electrical bound each confirm sets changes. Sync `docs/robot-movement.md` (and related copy) so at-min/at-max for hands matches this binding.

### 8. Empty-NVS defaults 80/100 all joints

Set every entry in `SERVO_SPECS` to `min=80`, `max=100` so settings defaults and boot clamps match. Midpoint is 90° — aligns with Stage A horn seating and guided center park when ranges are still defaults. Update UI `DEFAULT_SERVO_RANGES`, API/docs tables, and any tests that hard-code the old stock arrays.

**Alternatives:** Keep wide stock bands and only tighten in the wizard UI (rejected — empty NVS / skip would still expose unsafe-wide clamps before guided calibration). Per-joint asymmetric defaults (rejected — user asked for the same 80/100 on all).

### 9. Hub labels, hierarchy, and copy UX

Hub actions in this order: **Calibrate servos** (`primary`) → **Assembly support** (secondary / mid visual weight — new `variant="secondary"` or hub-scoped CSS) → **Skip** (default/quiet, last). Use one short intro, then put each action’s “when to use” guidance inside the button via title + hint. Do not repeat the same three choices in a separate list.

Instruction copy in hub, assembly, and guided steps uses `<strong>` (or equivalent) on landmarks and stop conditions only — not whole paragraphs. Guided `GUIDED_STEPS` copy becomes rich text (JSX / Preact nodes), not plain strings only.

Assembly identify: mark each joint as done after a successful identify wiggle (button state / check) until leaving assembly; Done CTA reads clearly (e.g. **Done · back to hub**). Identify buttons stay disabled until successful move-all-to-90° in that assembly visit (see §4).

Guided progress: keep the active role once as the section heading (`Body · center`), then show `Servo calibration · 1 of 10` plus a small internal progress bar. This is separate from the outer five-step wizard progress. Confirm labels: **Confirm center** / **Confirm min** / **Confirm max** (or hand landmark wording); final confirm is **Save and continue**. Demo after min still runs; omit “demo range” from the button. Skip when current ranges match stock 80–100: soft hint that defaults stay narrow (no modal). Do not show a final ranges-summary box.

**Alternatives:** Keep “Assembly: 90° + identify” (rejected — opaque). Equal-weight buttons (rejected — primary path unclear). Full-sentence bold (rejected — reduces scanability). Blocking skip modal (rejected — friction for re-flash / known-good robots).

### 10. Calibration guidance and wizard feedback

Label the calibration track with the current saved bounds (`Saved range: 80°–100°`) so the orange band has explicit meaning. Add concise per-substep target metadata (for example, `Target: straight forward` or `Target: over the keyboard`) immediately above the nudge controls.

Keep Back controls visually neutral and consistent across wizard screens while preserving a visible focus indication. Make Screen’s **Rotate 180°** action secondary because **Next** owns primary navigation, and display the current preview orientation (`Normal` / `Rotated 180°`). RGB test/mapping and Speaker playback show successful completion state after their device action succeeds.

On short mobile viewports, keep wizard action footers reachable with a sticky bottom treatment, safe-area inset, opaque background, and matching content padding so actions never cover controls.

### 11. Guided drafts and mirrored-range validation

Keep a session-only draft angle per guided substep. Nudge updates that substep’s draft; navigating Back moves to and displays the prior draft instead of recomputing its entry angle. Drafts reset when guided calibration is left or restarted and are never persisted.

For Body, Neck, and Head min confirmation, calculate `nextMax = 2 × center − min`, then validate the complete electrical range before mutating calibration state or moving a servo. Existing `min < center` validation remains. If `nextMax > 180`, show an error, keep the user on the min substep, leave ranges unchanged, and do not run the max demo.

**Alternatives:** Clamp `nextMax` to 180 (rejected — silently breaks symmetry). Save then report the invalid range (rejected — state and physical demo would already be wrong). Recompute entry position on Back (rejected — discards user work).

## Risks / Trade-offs

- **[Risk] Long blocking demo moves on slow phones / flaky AP** → Mitigation: keep one joint moving; disable double-submit; surface existing failure + restore angle pattern from tests.
- **[Risk] User sets min at or above session center so mirrored max is invalid** → Mitigation: reject confirm when `min >= center` (implies `max <= min`); status error like today’s min/max checks.
- **[Risk] An off-center center and low min mirror above electrical 180°** → Mitigation: validate `nextMax <= 180` before state mutation or demo.
- **[Risk] Wizard tests tightly coupled to old “Parts are on →” entry** → Mitigation: rewrite `ui/test/wizard.test.tsx` helpers around hub actions; keep save-payload assertions.
- **[Trade-off] Strict forward vs power-user retune** → Accept for v1; hub allows re-entering guided later in the same session if they Back from OLED… today Back from OLED re-enters ranges — preserve ability to re-open guided from hub when returning to servos step.

## Migration Plan

- Pure UI embed: ships with next firmware UI build (`pio run` embeds `ui/`). No NVS migration.
- Users mid-old-wizard: only relevant before flash; after flash, new hub.
- Rollback: revert UI change; NVS ranges remain compatible.

## Open Questions

None that block implementation.
