# Design

## Context

See proposal.md — Why. Eye styles plug in via `EyeStyleRenderer` (`styles/*.cpp` → `registry.cpp` + `eye_style_ids.cpp`). Pose/blink live in `display/eyes/core`; `dots` is the closest precedent: `needsPoseUpdate` + `needsBlink`, paint from `renderEye` + `blinkOpenAmount`, dead X via `drawDeadXEyes`. Settings validate uses registered ids only. Config `EYES_STYLES` and `docs/api.md` list the strings.

Canonical open look (from the reference silhouette): solid white filled shapes on black — pointed high outer tips, lower rounded inners, concave brow notch on top, smooth convex bottom, mirrors, no pupil/outline. Earlier pupil-notch frames are out of v1 scope.

## Goals / Non-Goals

**Goals:**

- Ship `evil` as a sixth registered style with dots-tier motion coupling
- Procedural draw that reads as the reference on 128×32
- Keep settings / HTTP / UI / docs in sync in one change

**Non-Goals:**

- New `POST /anim` names or servo/audio variants
- Kawaii-style mood director or kaomoji expression remap
- Bitmap/RLE asset pack or flash-heavy frame library
- Per-mode geometry redesigns beyond height/position from classic pose

## Decisions

### D1. Style id `evil`
Short, matches the persona, fits NVS/JSON string lists. Alternatives `menace` / `scowl` rejected as vaguer.

### D2. Follow dots architecture
One `drawEvil` in `styles/evil.cpp`, `kEvilEyeStyle` with `needsPoseUpdate`/`needsBlink` true. Scale/place a mirrored polygon (or filled path) from each rendered `Eye`. Height ≤ 0 → skip draw. Dead X hold → `drawDeadXEyes` on boxes centered like dots. Alternatives: static full-screen bitmap (no glance/blink fidelity); kawaii mood stack (overkill for “always evil”).

### D3. Procedural geometry, not assets
Encode swoosh as polygon / triangle fan / mask scaled to the eye box so blinks shrink cleanly. Pixel-perfect PNG copy deferred unless procedural fails visual check on device. Alternative: host-generated masks like expression RLE — more pipeline, unnecessary for one shape.

### D4. Docs and gallery in the same change
Update `docs/api.md`, UI ApiReference, Config select, styles README. Add a `web/eyes` card + preview path (cover stays optional/hidden). Keeps “list evil” requirement honest for the public gallery.

### D5. No settings schema version bump
New enum member only; unknown old firmware ignores / rejects; new firmware accepts. Factory default stays `classic`.

## Risks / Trade-offs

- [Swoosh hard to fill with Adafruit primitives] → Start with triangle/polygon fill or row-scan inside the eye AABB; fall back to a small PROGMEM mask if jagged
- [Look too soft when height-scaled during blink] → Prefer vertical squash of the same outline over morphing to classic ovals
- [Preview JS drifts from firmware] → Approximate like other styles; note gallery is not a C++ port
- [Flash/CPU on fill] → Profile against dots; keep fill bounded to eye boxes only

## Migration Plan

- Flash firmware that includes `evil`; set via Config or `POST /settings {"eyes_style":"evil"}`
- Rollback: flash prior firmware, or set another style; orphaned `evil` in NVS → validate fails on old builds that only know five ids — user re-selects a known style (same as any unknown style string)

## Open Questions

- Exact polygon control points vs on-device tweak after first flash (does not change requirements)
