# Proposal

## Why

Setup AP servo calibration today is a free-form joint tab UI after a single “move all to 90°” step. Assembly needs clear horn-centering and per-servo identification; a fully assembled robot needs a guided, landmark-based range pass. The current flow mixes those jobs and does not walk the user through safe limits in a dependable order.

## What Changes

- Replace the forced horns → free-form ranges sequence with a **servos hub** on wizard step 1:
  - **Calibrate ranges** — guided Stage B (assembled robot)
  - **Skip calibration** — leave existing `servo_mins` / `servo_maxs` (NVS or defaults); advance to the next wizard step
  - **Assembly** — Stage A (center at 90° + identify); always returns to the hub
- Stage A: move all joints to 90°; identify each servo with a small ±3° wiggle around midpoint; no auto-entry into Stage B
- Stage B: strict forward wizard (Back = previous substep or hub): Body → Neck → Head → Hands; landmark copy; slow moves via existing `POST /setup/servo`
- Body/Neck/Head: set **center** then **min** with the same ±1° / ±5° / ±10° nudge buttons; center step starts at the midpoint of that joint’s current min/max; max is mirrored about the confirmed center (`2×center − min`). Center exists only for the guided session (client memory); runtime and NVS keep mid as the midpoint of saved min/max
- Hands: no centers; bench-proven bound binding (keyboard = right **min** + left **max**; upper = left **min** + right **max**) — swapped vs the earlier docs-aligned UI mapping after on-robot testing
- Persistence unchanged: same `POST /settings` with `servo_mins` / `servo_maxs` only — no center field in NVS or settings
- No HTTP API changes; no firmware calibration logic changes

## Capabilities

### New Capabilities

- `setup-servo-calibration`: Setup AP wizard servo hub, assembly centering/identify, guided range calibration, and skip behavior (UI flow only; same save endpoints and NVS fields)

### Modified Capabilities

- (none — project has no archived `openspec/specs/` capabilities yet)

## Impact

- **UI**: `ui/src/setup/` (`Wizard.tsx`, `PlaceParts.tsx`, `FindRanges.tsx`, `useCalibration.ts`, related helpers/components)
- **Tests**: `ui/test/wizard.test.tsx` (and any new setup calibration tests)
- **Docs**: assembly/setup hub wording; hand at-min/at-max in `docs/robot-movement.md` (and related) to match bench-proven guided polarity
- **Unaffected**: `POST /setup/servo`, `POST /settings` servo fields, NVS layout, firmware motion speeds, CLI
