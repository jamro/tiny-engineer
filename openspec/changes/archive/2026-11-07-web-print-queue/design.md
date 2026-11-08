# Design

## Context

See proposal.md for motivation. Today `web/print/index.html` is a single CTA to GitHub `3d_models/README.md`. Shop already persists `tiny-engineer.shop.servo` and CTA → `/print/`. Assembly viewer already uses Three.js via import map (`three@0.186.0`) and design colors in `web/js/assembly/colors.js` (`PartsSetA` copper, `PartsSetB` black, `LampDiffuser` white). Print-group membership is known from CAD extract / GLB extras. Source meshes live under `3d_models/parts/{servo}/{3mf,stl}/` (~15–16 MB per servo for both formats × three servos). Pages workflow already builds assembly GLB from STL at deploy time — same pattern fits mesh publish for Print.

## Goals / Non-Goals

**Goals:**

- Turn `/print/` into a Shop-parity stage tool: locked servo, color lanes, progress, live STL thumbs, dual downloads.
- Keep meshes out of git; publish at Pages deploy.
- Stay performant enough on phones (lazy load / shared renderer), matching STYLE.

**Non-Goals:**

- Print-bureau order wizard (aggregates).
- Static PNG preview pipeline.
- HD-1370A in UI.
- Auto-codegen `parts.json` from Fusion (manual sync OK for v1; optional later CI check).
- Changing Shop requirements.

## Decisions

### 1. Page composition (queue + testers)

```
+------------------------------------------------------------------+
| build-timeline (print current)                                   |
| eyebrow · h1 Print · lede                                        |
+------------------------------------------------------------------+
| VARIANT (read-only)                                              |
| label + size from parts.json · [Change in Shop Section →]        |
+------------------------------------------------------------------+
| PRINT FIRST                                                      |
| [ ] canvas | ServoSizingTester | [3mf] [stl]                     |
| [ ] canvas | ScrewSizingTest   | [3mf] [stl]                     |
+------------------------------------------------------------------+
| COPPER · PartsSetA                         progress n/N          |
| BLACK  · PartsSetB                         ...                   |
| WHITE  · LampDiffuser                      tip: thin walls       |
+------------------------------------------------------------------+
| progress · Clear checks · [Continue to Connect]                  |
| Advanced → 3d_models README                                      |
+------------------------------------------------------------------+
```

**Alternatives considered:** Fork Home vs Order first (deferred — user scoped home queue + downloads). Dual-lane parallel (bad mobile). Catalog-only without checks (weaker progress).

### 2. Data: `web/print/parts.json`

Same-origin JSON (mirror `cart.json` discipline):

- `groups[]` — id, label, finish, swatch, order (`PartsSetA`, `PartsSetB`, `white`)
- `testers[]` — id, label, files `{3mf,stl}`, `printFirst: true`
- `parts[]` — id, group, label, role, qty, files
- `servos` — sg90 / fs0307 / es9051 metadata + `filesBase` e.g. `files/sg90/`
- storage key names documented in JSON or constants matching Shop: read `tiny-engineer.shop.servo`; write checks to `tiny-engineer.print.checked`

Color membership MUST match assembly print groups (Head→A, Desk→B, LampDiffuser→white, …). `UpperArm` qty `2` (mirrored print; one mesh file). Omit `PartsSetA`/`PartsSetB` aggregate files from checklist rows.

**Alternatives considered:** Scrape README (fragile). Derive groups only from live GLB at runtime (ties Print to assembly model; harder offline reasoning). One JSON per servo (duplication).

### 3. Servo lock

Read Shop key only. Unknown / legacy `hd1370a` → fall back to `sg90` with a short status note + Shop link. No in-page picker.

### 4. Mesh publish (CI)

In `.github/workflows/pages.yml`, before `upload-pages-artifact`:

1. For each of `sg90`, `fs0307`, `es9051`: copy individual part + tester `.3mf` and `.stl` into `web/print/files/{servo}/` (exclude aggregate `PartsSetA`/`PartsSetB` unless a later service path needs them).
2. Optionally assert every `parts.json` file reference exists.
3. Do **not** commit `web/print/files/**` meshes (gitignore or leave untracked generated tree).

Local preview: document a small script or make target to sync files for `python -m http.server` / Pages-like serve.

**Alternatives considered:** Commit meshes (repo bloat). GitHub raw primary downloads (rejected — user chose same-origin). Release assets only (extra indirection).

### 5. Live STL previews

- Module script + import map like `web/assembly/index.html` (same Three version).
- Load with `STLLoader` from `filesBase + files.stl`.
- Material color = group swatch (reuse hex from `colors.js` or duplicate swatches in JSON).
- **Performance:** `IntersectionObserver` — load/render when row near viewport; pause/dispose or park when far; prefer a small shared renderer/pool (1–2 WebGL contexts) over 26 independent engines.
- Interaction: gentle auto-rotate; optional drag via OrbitControls on focus — keep light.
- Fallback: swatch + part name if WebGL/load fails; never block downloads/checks.
- Preview source = STL only; 3MF remains download-only (parsing ZIP/3MF in-browser out of scope).

**Alternatives considered:** Build-time PNG (rejected — user chose live). Per-row full Three app (too heavy). GLB slice from assembly model (wrong scale per servo folder; couples to SG90-only GLB today).

### 6. Implementation shape

- Rewrite `web/print/index.html` mounts (variant banner, testers, lanes, footer).
- New `web/js/print-queue.js` (or split preview helper) — vanilla ES module.
- CSS under `.page-print` in `base.css`, reuse shop checklist / panel patterns where possible.
- Beginner-friendly microcopy consistent with recent build-guide tone change.
- Docs: one-line pointer from `3d_models/README.md` / getting-started §Print to site `/print/` (optional but preferred).

## Risks / Trade-offs

| Risk | Mitigation |
| --- | --- |
| Large Pages artifact (~45 MB meshes) | Individuals only; no aggregates; monitor Pages size; cache CI if needed |
| Many WebGL contexts / jank on mobile | Observer + shared pool; pause offscreen |
| `parts.json` drifts from CAD color groups | Document sync with assembly `print_group`; optional later CI assert vs GLB extras |
| Local serve without CI copy → broken downloads/previews | Document sync script; status message if STL 404 |
| Shop/Print key mismatch | Hard-code same Shop key string; test cross-page |

## Migration Plan

1. Ship `parts.json` + UI; CI publish step; gitignore generated `files/`.
2. Stub replaced in place at `/print/` — no URL break.
3. Rollback: revert workflow copy + restore stub HTML if needed; Shop CTA still valid.

## Open Questions

- None that block specs/tasks: shared vs per-row renderer pooling is an implementation detail within the live-preview requirement.
