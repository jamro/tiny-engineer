# Proposal

## Why

GitHub Pages deploy always runs `scripts/assembly/build.sh` before publishing `web/`. A CAD extract, geometry check, Python dependency, or validation failure can block unrelated site updates (Flash, Screens, copy). Decouple rebuild cost from unchanged model inputs while keeping a failed rebuild blocking when the model actually needs regenerating.

## What Changes

- Pages deploy restores a cached stock SG90 GLB when assembly model inputs are unchanged, and skips `build.sh` (and its Python install) on cache hit.
- On cache miss, Pages still runs the full SG90 build + checks into `web/assembly/models/` and saves the result to the cache; a failed rebuild still fails the deploy.
- Docs state that Pages may reuse a cached GLB keyed by model inputs, not that every deploy rebuilds from scratch.
- No soft-fail / stale-on-broken-CAD path; no committing the GLB; no downloading CI workflow artifacts.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `assembly-viewer`: Pages deploy MAY publish a cached SG90 GLB when model inputs are unchanged; MUST rebuild (and fail the deploy on rebuild failure) when those inputs change or the cache misses. Still SG90-only; still no print staging.

## Impact

- `.github/workflows/pages.yml` — restore/save Actions cache around the assembly build step; conditional build + pip install.
- `docs/3d/assembly-viewer.md` (and any other place that claims every Pages deploy rebuilds the model).
- Spec delta under `assembly-viewer` (pipeline requirement / Pages scenario).
- CI `assembly` job unchanged (still always builds and uploads `assembly-<sha>`).
- Viewer runtime and `build.sh` itself unchanged.
