# Design

## Context

See proposal.md — Why. Today wizard step 1 forces `PlaceParts` (horns) then `FindRanges` (joint tabs). State lives in `useCalibration` (`phase: horns | ranges`, `joint`, `angles`, `ranges`). Moves go through `useSetupServo` → `POST /setup/servo` (~25°/s). Save on Next posts `servo_mins` / `servo_maxs` from in-memory ranges. No API or NVS shape change.

## Goals / Non-Goals

**Goals:**

- Hub-based entry replacing forced horns → tabs
- Stage A (assembly) and Stage B (guided ranges) as separate UI modes with clear return/skip rules
- Derive mirrored max in the client from a session-only center; demonstrate ranges with sequential setup moves
- Keep Vitest coverage for hub, skip, Stage A identify, Stage B order/symmetry/hands polarity, and save payloads

**Non-Goals:**

- Firmware changes, new HTTP routes, or altering `/setup/servo` / settings validation
- Persisting per-joint center angles (NVS, settings fields, or browser storage beyond the live UI session)
- Re-calibrating from the STA Config page
- CLI / serial parity for setup calibration
- Changing stock default ranges in `include/servos.h`

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

Center and min/hand substeps share the same ±1/5/10 nudge pad and angle readout. Entering a `*-center` step parks at `(ranges[joint].min + ranges[joint].max) / 2` from the ranges already held for the step (NVS or defaults) — not hard-coded electrical 90°. On confirm of a `*-center` step: store `centers[joint] = angle` in calibration hook state only. On confirm of a `*-min` step: reject if `min >= centers[joint]`; else `max = 2 * centers[joint] - min`. Min park may start near `center − 20`. Demo moves to max then back to `centers[joint]`, then auto-advance when moves succeed. Electrical 90° remains Stage A (horn seating) only.

**Alternatives:** Free tabs with a “recommended order” checklist (rejected — too easy to skip landmarks). Compute max only at save time without demo (rejected — user asked for visual sweep). Fixed electrical 90° as default guided center (rejected — stock/saved ranges already define a mid).

### 3. Session-only center on the client (not NVS)

Keep `centers` as in-memory state in `useCalibration` for the guided path. Discard when leaving guided mode or finishing save. Runtime already treats pose `0` as the midpoint of saved min/max, so a persisted center would be redundant and wrong if it drifted from `(min+max)/2`.

**Alternatives:** NVS/settings center field (rejected — unused after save, API churn). `sessionStorage` (unnecessary; hook state lives for the wizard session). Derive center only as `(min+max)/2` without a center step (rejected — user must aim the landmark before choosing min).

### 4. Reuse existing move helper; no new endpoints

Assembly identify: for index `i`, chain `90 → 93 → 87 → 90` (or equivalent ±3) via `useSetupServo`. Guided nudges and demos: same helper. Disable controls while a move is in flight (existing `perform` pending already surfaces status).

**Alternatives:** Client-side interpolated PWM (impossible without firmware). Faster `/test/servo` (wrong clamp + wrong mode).

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

## Risks / Trade-offs

- **[Risk] Long blocking demo moves on slow phones / flaky AP** → Mitigation: keep one joint moving; disable double-submit; surface existing failure + restore angle pattern from tests.
- **[Risk] User sets min at or above session center so mirrored max is invalid** → Mitigation: reject confirm when `min >= center` (implies `max <= min`); status error like today’s min/max checks.
- **[Risk] Wizard tests tightly coupled to old “Parts are on →” entry** → Mitigation: rewrite `ui/test/wizard.test.tsx` helpers around hub actions; keep save-payload assertions.
- **[Trade-off] Strict forward vs power-user retune** → Accept for v1; hub allows re-entering guided later in the same session if they Back from OLED… today Back from OLED re-enters ranges — preserve ability to re-open guided from hub when returning to servos step.

## Migration Plan

- Pure UI embed: ships with next firmware UI build (`pio run` embeds `ui/`). No NVS migration.
- Users mid-old-wizard: only relevant before flash; after flash, new hub.
- Rollback: revert UI change; NVS ranges remain compatible.

## Open Questions

None that block implementation. Minor copy polish for landmarks can happen during apply.
