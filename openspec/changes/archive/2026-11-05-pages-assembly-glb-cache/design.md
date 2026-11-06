# Design

## Context

See proposal.md — Why. Today `.github/workflows/pages.yml` is a single `deploy` job that always `pip install`s assembly deps and runs `scripts/assembly/build.sh web/assembly/models` before uploading `web/`. The GLB is gitignored; CI already builds and uploads `assembly-<sha>` but Pages does not consume it. Spec `assembly-viewer` requires SG90-only publish; this change softens “rebuild every deploy” to “rebuild when inputs change.”

## Goals / Non-Goals

**Goals:**

- Skip extract/checks/GLB work on Pages when model inputs are unchanged (Flash/Screens/docs-only deploys).
- Keep cache-miss / input-change rebuilds strict: failed `build.sh` fails deploy.
- Keep one Pages job and SG90-only output path `web/assembly/models/sg90.glb`.

**Non-Goals:**

- Soft-fail or publish site with missing/stale GLB when rebuild fails (option B).
- Commit the GLB or wire Pages to CI `download-artifact` / `workflow_run`.
- Change `build.sh`, the viewer, or the CI `assembly` job behavior.
- Split Pages into multiple jobs.

## Decisions

1. **GitHub Actions cache keyed by model-input hash**  
   Use `actions/cache` with `path: web/assembly/models` and an exact key derived from `hashFiles` over every input that affects the published GLB or the checks `build.sh` runs before writing it:
   - `3d_models/cad/**`
   - `3d_models/parts/*/stl/**`
   - `3d_models/parts/*/step/**`
   - `3d_models/fusion/TinyEngineerTools/servos.json`
   - `scripts/assembly/**` (extractor, fasteners, checks, hardware STLs, `requirements.txt`, `build.sh`)
   - `web/assembly/steps.json` (step/clipping/pose checks; site ships steps beside the model)  
   **Why not CI artifact:** Pages and CI are separate workflows; same-SHA download races with CI and couples release/`workflow_dispatch` to a CI run. Cache is local to Pages and keyed by content, not SHA.  
   **Why not `restore-keys` fallback:** A partial prefix restore could serve a GLB built from different inputs. Exact key only.

2. **Conditional pip + `build.sh` on cache miss**  
   On `cache-hit == true`, skip Python 3.13 setup for assembly, `pip install`, and `build.sh`; restored `web/assembly/models/sg90.glb` is enough. On miss, keep today’s install + build into that directory so the post-job cache save stores the new GLB. Flash catalog (Python 3.11) stays unconditional.

3. **Docs only, not viewer code**  
   Update `docs/3d/assembly-viewer.md` so it no longer claims every Pages deploy rebuilds from CAD. No runtime change.

## Risks / Trade-offs

- [Cold cache / eviction + flaky PyPI] → Deploy still fails until rebuild succeeds; same as today for that case. Mitigation: accept; optional later retry is out of scope.
- [Forgotten input in `hashFiles`] → Stale GLB next to new steps/CAD. Mitigation: key list mirrors Pages path filters plus `steps.json` and all of `scripts/assembly/**`; task to review against `build.sh` deps.
- [Cache hit skips checks that would catch a latent bug in scripts] → CI `assembly` job still always rebuilds on those path changes. Mitigation: rely on CI for validation; Pages cache is publish optimization only.
- [First deploy after merge has no cache] → One full rebuild; expected.

## Migration Plan

1. Land workflow + doc + spec archive as usual.
2. First Pages run after merge: cache miss → full build → populate cache.
3. Later web-only deploys: cache hit → no assembly rebuild.
4. Rollback: remove cache step and restore unconditional `build.sh` in `pages.yml`.

## Open Questions

- None that block implementation; exact `actions/cache` major version follows whatever the repo already uses elsewhere (or current `@v4` if unused).
