# Design

## Context

- `anim::facePose()` returns the saved pose or talking’s rest (`head 1/7`, neck/body `0`). Attention’s unset rest is different: head `ATTENTION_HEAD_READY` (`6/35`), neck/body `0`.
- Attention phases in `src/animation/attention.cpp`: PrepPose → PlayAudio → AwaitInput. Body is set once in prep; neck/head take relative offsets during audio and wait; hands stay on `ATTENTION_HAND_*`.
- Talking caches `anim::facePose()` once in `startTalking()` and offsets with `servoSaturateNorm`.

## Goals / Non-Goals

**Goals:**
- Zero behavior change when `face_pose` is not set.
- When set, attention faces the user for prep, audio cues, and the waiting loop.

**Non-Goals:**
- welcome, ring, typing, reading, thinking.
- Changing `anim::facePose()` or the Face me UI preview (stays talking).
- Mid-animation live update of a newly saved pose (same as talking: next start).

## Decisions

### D1. Attention-specific rest helper, not bare `anim::facePose()`
A local `attentionRest()` reads `settingsFacePose()`. If unset, return `{ATTENTION_HEAD_READY, ATTENTION_NECK_MID, ATTENTION_BODY_MID}`. If set, return the saved values. Cache once in `startAttention()` as `g_face`.

Calling `anim::facePose()` alone would change unset attention head from `6/35` to `1/7`.

### D2. Offsets stay relative; clamp with `servoSaturateNorm`
Prep targets `g_face` plus hand parks. Audio and await add the existing glance/nod/tilt constants to `g_face.head` / `g_face.neck`. Wrap every head/neck/body target in `servoSaturateNorm`. Hands unchanged.

### D3. Supersede the face-me “ignore attention” scenario
Edit `openspec/changes/face-me/specs/face-pose/spec.md` so “other animations ignore” lists typing, reading, thinking, welcome, ring (drop attention). This change’s delta adds the attention requirement.

## Risks / Trade-offs

- [Face pose near ±1 clips glances] → same as talking; clamp keeps targets valid.
- [Two open deltas for face-pose] → face-me edit keeps the ignore list accurate until archive; this change owns the attention requirement.

## Migration Plan

No migration. Unset devices unchanged. Set pose applies the next time attention starts.
