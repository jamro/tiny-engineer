# Design

## Context

See proposal.md — Why. Today wizard step 1 forces `PlaceParts` (horns) then `FindRanges` (joint tabs). State lives in `useCalibration` (`phase: horns | ranges`, `joint`, `angles`, `ranges`). Moves go through `useSetupServo` → `POST /setup/servo` (~25°/s). Save on Next posts `servo_mins` / `servo_maxs` from in-memory ranges. No API or NVS shape change.

## Goals / Non-Goals

**Goals:**

- Hub-based entry replacing forced horns → tabs
- Stage A (assembly) and Stage B (guided ranges) as separate UI modes with clear return/skip rules
- Derive mirrored max in the client; demonstrate ranges with sequential setup moves
- Keep Vitest coverage for hub, skip, Stage A identify, Stage B order/symmetry/hands polarity, and save payloads

**Non-Goals:**

- Firmware changes, new HTTP routes, or altering `/setup/servo` / settings validation
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
hand-right-max → hand-left-min → hand-left-max → hand-right-min
```

On confirm of a `*-min` step: `max = 180 - min` (center fixed at 90). Demo steps issue moves to max then to 90 and auto-advance (or require Continue after moves finish — prefer auto-advance when moves succeed so the path stays short).

**Alternatives:** Free tabs with a “recommended order” checklist (rejected — too easy to skip landmarks). Compute max only at save time without demo (rejected — user asked for visual sweep).

### 3. Reuse existing move helper; no new endpoints

Assembly identify: for index `i`, chain `90 → 93 → 87 → 90` (or equivalent ±3) via `useSetupServo`. Guided nudges and demos: same helper. Disable controls while a move is in flight (existing `perform` pending already surfaces status).

**Alternatives:** Client-side interpolated PWM (impossible without firmware). Faster `/test/servo` (wrong clamp + wrong mode).

### 4. Wizard footer ownership

Hub: no Next into Screen until Skip or finishing guided (Skip acts as advance). Assembly: Back/Done → hub; hide the main wizard Next (same idea as today’s `placingParts` footer hide). Guided: Back = substep back; primary = Confirm / Next substep; final confirm triggers existing servos `save()` then `stepIndex + 1`.

Skip from hub: call the same save path with current `ranges` (loaded from settings), then advance — matches “mechanism unchanged.”

### 5. File split

Prefer small Preact files already used in `ui/src/setup/`:

- Hub component (new)
- Adapt `PlaceParts` → assembly mode (+ per-servo Identify)
- Replace or heavily slim `FindRanges` → guided substep view + copy
- Expand `useCalibration` (or split `useGuidedCalibration`) for mode + substep index + range updates

Keep `SERVO_NAMES` / index mapping; guided order uses body=4, neck=1, head=0, hands 3 then 2 as specified.

### 6. Docs polarity as single source for hand bounds

UI copy and which button sets min vs max MUST follow `docs/robot-movement.md`. Do not invert for “physical up/down” storytelling beyond labeling the landmarks.

## Risks / Trade-offs

- **[Risk] Long blocking demo moves on slow phones / flaky AP** → Mitigation: keep one joint moving; disable double-submit; surface existing failure + restore angle pattern from tests.
- **[Risk] User sets body min past 90° so mirrored max is invalid (`min >= max`)** → Mitigation: reject confirm when `min >= 90` (or `max <= min`); status error like today’s min/max checks.
- **[Risk] Wizard tests tightly coupled to old “Parts are on →” entry** → Mitigation: rewrite `ui/test/wizard.test.tsx` helpers around hub actions; keep save-payload assertions.
- **[Trade-off] Strict forward vs power-user retune** → Accept for v1; hub allows re-entering guided later in the same session if they Back from OLED… today Back from OLED re-enters ranges — preserve ability to re-open guided from hub when returning to servos step.

## Migration Plan

- Pure UI embed: ships with next firmware UI build (`pio run` embeds `ui/`). No NVS migration.
- Users mid-old-wizard: only relevant before flash; after flash, new hub.
- Rollback: revert UI change; NVS ranges remain compatible.

## Open Questions

None that block implementation. Minor copy polish for landmarks can happen during apply.
