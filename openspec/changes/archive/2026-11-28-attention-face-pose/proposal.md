# Proposal

## Why

The face-me pose already points the robot at the user for talking, but attention still aims at a fixed desk-center pose. After a continuous timeout, the robot asks for input while looking the wrong way. Attention is the next user-facing animation that should reuse the saved pose.

## What Changes

- When `face_pose` is set, the attention animation rests prep, audio motion, and the waiting loop on that head/neck/body pose, with the same relative glances, nods, and hand cues as today (clamped to −1..1).
- When `face_pose` is not set, attention keeps the stock `ATTENTION_*` poses (no behavior change).
- Docs and the face-pose OpenSpec delta stop saying attention ignores the pose.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

(none — `face-pose` is not in main specs until `face-me` archives; this change adds requirements under that capability path)

### Capability path (delta under open face-me work)

- `face-pose`: ADDED requirements — attention uses the saved face pose when set; unset keeps stock `ATTENTION_*`. The open `face-me` delta’s “other animations ignore” list drops attention so the two changes agree.

## Impact

- Firmware: `src/animation/attention.cpp` (cache rest at start; rebase prep/audio/await on face pose).
- Docs: `docs/robot-movement.md`, `docs/api.md`.
- OpenSpec: supersede the conflicting scenario in the still-open `face-me` change so both deltas agree.
