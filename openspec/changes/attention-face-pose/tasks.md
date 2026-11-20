# Tasks

## 1. Firmware

- [x] 1.1 In `src/animation/attention.cpp`, add `attentionRest()` / cache `g_face` in `startAttention()`, and rebase prep, audio, and await head/neck/body targets on `g_face` with `servoSaturateNorm`
- [x] 1.2 Run `pio run` and fix failures

## 2. Docs

- [x] 2.1 Update `docs/robot-movement.md` Face me section: talking and attention use the pose; unset keeps stock attention constants
- [x] 2.2 Update `docs/api.md` `face_pose` field and attention animation row to mention face me when set

## 3. OpenSpec coherence

- [x] 3.1 Keep `openspec/changes/face-me/specs/face-pose/spec.md` ignore-list without attention (done with this change)
- [x] 3.2 Run `openspec validate attention-face-pose --strict`
