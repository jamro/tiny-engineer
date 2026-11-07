# Design

## Context

See proposal.md — Why. `/flash/` today is release picker + `esp-web-install-button` + image map, with seating/USB hints only in panel foot callouts. `/assembly/` already loads `models/sg90.glb` via Three + `createStage` (`web/js/assembly/stage.js`) and slides parts with `createBuild` / `steps.json`. Flash happens **before** desk assembly (`docs/flash.md`, getting-started): bench electronics only. Assembly step `#esp32-seat` seats through `DeskTop` — wrong scene for flash. GLB nodes needed: `MainControlBoard`, `ESP32-C3-Zero`, `ESP32-pin-headers`. No USB cable mesh. PCBA prepare copy is the plain-language precedent.

## Goals / Non-Goals

**Goals:**

- Thin flash-only prep module that reuses stage + shared GLB.
- Two guided steps with seat animation and USB port distinction.
- Prep section first in page order; install path unchanged.

**Non-Goals:**

- Deep-linking or embedding the full assembly build player.
- New GLB / CAD pipeline or extra published models.
- Teaching header soldering on `/flash/`.
- Changing esp-web-tools, release manifests, or firmware.
- Rewriting `docs/**` tone (optional one-line pointer only).

## Decisions

### 1. Dedicated `flash-prep.js`, not `createBuild`

Own a small controller under `web/js/` (e.g. `flash-prep.js`) that imports `createStage` (and colors/helpers as needed) from `web/js/assembly/`. Hard-code two steps; do not flatten `steps.json` or mount assembly outline/play/center UI.

**Alternatives considered:** Reuse `createBuild` + a flash-only `steps.json` fragment (heavy coupling, desk-aware ghosting). Iframe `/assembly/#…` (wrong scene, bad UX). Rejected.

### 2. Shared GLB URL, filter to electronics

Load `../assembly/models/sg90.glb`. Keep only the three electronics nodes (plus any fastener children attached to them if present); dispose or never add the rest to the stage content group. Frame the board bbox.

**Alternatives considered:** Separate mini GLB in CI (extra pipeline). Static PNG renders (less “mocno wizualnie”). Rejected for this change.

### 3. Step 1 starts with module elevated; Next seats then advances

On first paint / step 1: board visible, ESP32 + headers offset along +Z (~30 mm, matching assembly seat motion scale), then ease into assembled pose (~0.9 s, same ease family as `build.js`). Next on step 1 finishes seating if mid-animation, then shows step 2. Prev on step 2 returns to seated (or re-plays elevate→seat optionally — prefer seated snapshot for Prev to avoid churn).

**Alternatives considered:** Start already seated, animate only on first Next (weaker). Autoplay both steps without Next (harder to read). Recorded as default: elevate on step 1 entry.

### 4. USB distinction via world→screen callouts

Step 2: HTML overlays pinned by projecting approximate USB-C world points (board receptacle end; module USB end from known frame / bbox heuristics in `extract.py` orientation: module USB toward board +X). Labels: board = correct, module = incorrect. No cable mesh.

**Alternatives considered:** Emissive material on guessed mesh faces (fragile on merged STL). Separate annotation meshes in CAD (out of scope). Rejected for v1.

### 5. Page layout

After `build-head`, full-width prepare panel: 3D stage + step card (title, body, Prev/Next, step counter). Then existing `.hero` install | image map. Import map for `three` on flash page like assembly. Shorten install foot callouts that duplicate prep; keep Chrome/Edge + data-cable note near install.

**Alternatives considered:** Side-by-side prep inside `.hero` (prep fights install width). Prep modal before unlock install (extra friction). Rejected.

### 6. Failure mode

If WebGL or GLTF load fails: hide or collapse canvas, keep step text + nav, status line with short error; `flash-status.js` install flow unaffected.

### 7. Voice

Prep strings match build-guide beginner voice (job first). May say “ESP32” once after “module” for recognition. No JLCPCB / pin-net jargon in primary step body.

## Risks / Trade-offs

- [USB mark unreadability in mesh] → Mitigation: step-1 copy always names the USB mark; camera angles toward silk side; callout optional if silk glyphs are weak.
- [BBox heuristics for USB ends drift if board mesh changes] → Mitigation: isolate projection helpers with constants next to seat offset; manual check after GLB rebuilds; fallback to static corner anchors relative to board bbox.
- [Loading full 2.5 MB GLB on flash] → Mitigation: same asset assembly already uses; filter after load; optional later: deferred load until prep visible (not required for v1).
- [Duplication with assembly seat motion] → Accepted: small duplicated ease/offset constants better than coupling to `steps.json`.
- [web-build-guide not archived yet] → Prep requirements self-contain beginner voice; no blocked dependency on that archive.

## Migration Plan

- Static Pages deploy only; no firmware/NVS.
- Rollback = revert flash HTML/JS/CSS; GLB untouched.
- No localStorage keys required for prep (optional later: remember completed prep — out of scope).

## Open Questions

- Exact board/module USB world anchors after first GLB inspect in apply (constants, not spec change).
- Whether install-panel foot callouts shrink to one line or stay as secondary reminder — prefer shrink during apply if prep is clear.
