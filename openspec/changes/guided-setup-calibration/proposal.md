# Proposal

## Why

Setup AP servo calibration today is a free-form joint tab UI after a single “move all to 90°” step. Assembly needs clear horn-centering and per-servo identification; a fully assembled robot needs a guided, landmark-based range pass. The current flow mixes those jobs and does not walk the user through safe limits in a dependable order.

## What Changes

- Replace the forced horns → free-form ranges sequence with a **servos hub** on wizard step 1:
  - **Calibrate ranges** — guided Stage B (assembled robot)
  - **Skip calibration** — leave existing `servo_mins` / `servo_maxs` (NVS or defaults); advance to the next wizard step
  - **Assembly** — Stage A (center at 90° + identify); always returns to the hub
- Stage A: move all joints to 90°; identify each servo with a small ±3° wiggle around midpoint; no auto-entry into Stage B
- Stage B: strict forward wizard (Back = previous substep or hub): Body → Neck → Head → Hands; mirrored max from center for body/neck/head; landmark copy; slow moves via existing `POST /setup/servo`
- Hands: electrical min/max per [`docs/robot-movement.md`](../../../docs/robot-movement.md) (keyboard = left min + right max; upper = left max + right min); no hand centers
- Nudges stay ±1° / ±5° / ±10°
- Persistence unchanged: same `POST /settings` with `servo_mins` / `servo_maxs` when leaving the servos step after calibrate or skip
- No HTTP API changes; no firmware calibration logic changes

## Capabilities

### New Capabilities

- `setup-servo-calibration`: Setup AP wizard servo hub, assembly centering/identify, guided range calibration, and skip behavior (UI flow only; same save endpoints and NVS fields)

### Modified Capabilities

- (none — project has no archived `openspec/specs/` capabilities yet)

## Impact

- **UI**: `ui/src/setup/` (`Wizard.tsx`, `PlaceParts.tsx`, `FindRanges.tsx`, `useCalibration.ts`, related helpers/components)
- **Tests**: `ui/test/wizard.test.tsx` (and any new setup calibration tests)
- **Docs** (copy sync only if needed): assembly/setup mentions of the old horns→tabs flow; not API contract docs unless wording refers to the old UI
- **Unaffected**: `POST /setup/servo`, `POST /settings` servo fields, NVS layout, firmware motion speeds, CLI
