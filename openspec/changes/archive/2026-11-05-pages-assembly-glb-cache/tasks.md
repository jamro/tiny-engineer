# Tasks

## 1. Pages workflow cache

- [x] 1.1 In `.github/workflows/pages.yml`, add `actions/cache@v4` for `web/assembly/models` with an exact key from `hashFiles` over `3d_models/cad/**`, `3d_models/parts/*/stl/**`, `3d_models/parts/*/step/**`, `3d_models/fusion/TinyEngineerTools/servos.json`, `scripts/assembly/**`, and `web/assembly/steps.json` (no `restore-keys`); verify the step has an `id` and does not set restore-keys
- [x] 1.2 Gate Python 3.13 setup, `pip install -r scripts/assembly/requirements.txt`, and `scripts/assembly/build.sh web/assembly/models` on cache miss only; keep flash catalog (Python 3.11) unconditional — verify YAML conditions use the cache step `cache-hit` output and a cold-path still runs `build.sh`
- [x] 1.3 Confirm a failed `build.sh` on cache miss still fails the job (no `continue-on-error`) — verify the build step has no soft-fail flags

## 2. Docs

- [x] 2.1 Update `docs/3d/assembly-viewer.md` so Pages is described as publishing `models/sg90.glb` via rebuild-or-cache keyed by model inputs, not as rebuilding from CAD on every deploy — verify the doc no longer claims every Pages deploy runs a full CAD rebuild, and still points at `pages.yml` / `build.sh` for local/CI builds
