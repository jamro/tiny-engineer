# Design

## Context

See proposal.md — Why. Today `/shop/` coaches KiCad + Fabrication Toolkit export; `hardware/**/production/` is gitignored; CI already installs KiCad 10 lite AppImage for ERC/DRC (`check_pcb.py`) but does not export fab packages. Pages deploys the `web/` tree and currently path-filters omit `hardware/boards/**`. Project options file `fabrication-toolkit-options.json` enables `AUTO TRANSLATE` / `AUTO FILL` for the JLCPCB Fabrication Toolkit plugin (installed locally as `com_github_bennymeg_JLC-Plugin-for-KiCad`, which exposes a CLI).

## Goals / Non-Goals

**Goals:**

- Generate Gerber ZIP + `bom.csv` + `positions.csv` during Pages deploy into `web/shop/fab/main-control-board/`.
- Prefer the same Fabrication Toolkit outputs builders are told to use today (JLCPCB column conventions + rotation auto-translate).
- Wire shop UI + docs to download-first; keep self-export documented.
- Fail deploy if generation fails.
- Track `main` board sources only (no Release pin in this change).

**Non-Goals:**

- Committing fab outputs under `hardware/**/production/`.
- Bot-commits of generated files onto `main`.
- Tag/Release-hosted fab packages.
- Automating JLCPCB login, quote, or stock substitution.
- Other boards in v1.
- Guaranteeing first-try perfect orientation without human JLC preview (AUTO TRANSLATE reduces, does not eliminate, review).

## Decisions

### 1. Build-time into `web/shop/fab/` (not git, not Release)

Pages job runs a generator that writes:

```
web/shop/fab/main-control-board/
  gerbers.zip          # stable public name (copy/rename from Toolkit archive if needed)
  bom.csv
  positions.csv
  SOURCE.txt           # optional: git sha, generator id/version — nice for support
```

Deploy artifact remains `path: web`. Nothing under `hardware/**/production/` is published as source of truth.

**Alternatives considered:** Commit `web/shop/fab/` on main (stale + PR noise). Release assets (user deferred). CI artifact-only (expires; bad builder UX).

### 2. Generator: Fabrication Toolkit CLI first

Primary path: pin a Fabrication Toolkit release, run its CLI against `main-control-board.kicad_pcb` with `--autoTranslate` (and align with project options: auto-fill, exclude-DNP as today). Copy/normalize outputs into the public filenames above.

**Spike (blocking for apply):** confirm headless run on GitHub-hosted Ubuntu with a KiCad that provides `pcbnew` Python (lite AppImage used for `kicad-cli` may be insufficient). If Toolkit cannot run:

- **Fallback:** `kicad-cli` gerbers + `sch export bom` (LCSC-mapped labels) + `pcb export pos` with header remap to Toolkit-like columns; document rotation risk; user validates first JLCPCB order before calling the path “done.”

Do not invent BOM rows; LCSC must come from schematic fields.

**Alternatives considered:** Manual maintainer export only (rejected — user wants full auto). Pure `kicad-cli` without Toolkit (weaker rotation parity).

### 3. Where the step lives: Pages workflow (+ shared script)

Implement `scripts/` entrypoint (e.g. `scripts/fab/export_main_control_board.sh` or `.py` wrapper) callable locally and from CI. Invoke it from `.github/workflows/pages.yml` before `upload-pages-artifact`.

Extend Pages `paths:` to include at least:

- `hardware/boards/main-control-board/**`
- the new script path(s)
- keep existing `web/**` triggers

Also keep `workflow_dispatch` for manual refresh.

**Alternatives considered:** Separate workflow that pushes to Pages (more moving parts). Generate in `ci.yml` pcb job and pass artifact into Pages (cross-workflow coupling; optional later optimization).

### 4. Shop UI

`cart.json` `jlcpcb` block gains download URLs (relative same-origin) and revised `steps` (download → JLCPCB both-sides → review → arrive). Shop JS renders download buttons/links next to the lane. Full guide URL stays.

Optional: if files missing in local static preview, show a short “generated on deploy” note rather than broken primary path — production Pages must have files (fail-closed).

### 5. Docs

`docs/hardware/order-main-control-board.md`: lead with “download from `/shop/`”; keep KiCad + Toolkit as alternate. Note published package tracks site deploy / `main`, not a release tag. Clarify JLCPCB stock/orientation review still required.

### 6. Caching

Cache KiCad/Toolkit install layers aggressively (similar to pcb CI AppImage cache). Fab output itself is cheap (~hundreds of KB); correctness > micro-optimizing skip-generate. Skipping generation when board hash unchanged is optional later — not required for v1.

## Risks / Trade-offs

| Risk | Mitigation |
| --- | --- |
| Lite AppImage lacks `pcbnew` for Toolkit CLI | Spike early; fallback to fuller KiCad install or `kicad-cli` remap path |
| Rotation / CPL mismatch vs hand Toolkit | Prefer Toolkit + AUTO TRANSLATE; user validates real JLCPCB order before declaring success |
| Pages deploy slower / flakier (KiCad download) | Cache extracts; pin versions; fail closed |
| Local `web/` preview has no fab files | Document “run export script” or accept deploy-only; do not commit production under hardware |
| Stale mental model (“files in git”) | Docs + SOURCE.txt; never commit hardware production as publish source |
| JLCPCB UI / BOM format drift | Spec outcomes not button names; keep full guide; pin Toolkit version |

## Migration Plan

1. Land generator script + Pages wiring behind deploy; verify artifact contains `/shop/fab/main-control-board/*`.
2. Update shop copy/links once files exist on a successful Pages run (or same PR if script runnable in CI check).
3. Update order doc to download-first.
4. Maintainer places a test JLCPCB order (or dry-run upload through BOM/CPL review screens) and files any rotation/BOM issues as follow-ups.
5. Rollback: revert Pages fab step + shop steps to KiCad instructions; site returns to previous coaching path.

## Open Questions

- Exact public Gerber filename (`gerbers.zip` vs Toolkit archive name) — pick during implement; keep cart.json URLs in sync.
- Whether `SOURCE.txt` is required or optional — optional nicety; not a spec MUST.
