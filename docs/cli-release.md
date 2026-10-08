# Releasing the `tiny-engineer` npm CLI

The host CLI (`packages/tiny-engineer-cli`, npm package **`tiny-engineer`**) is versioned and published **separately** from firmware GitHub Releases.

Firmware tags (`v*`) build flash artifacts via `.github/workflows/release.yml`. They do **not** publish npm. See [CONTRIBUTING.md](../CONTRIBUTING.md).

## Versioning

| Artefact | Where the version lives | Git tag |
|---|---|---|
| Firmware / web flash | `git describe` → `FW_VERSION` | `v0.2.0` (existing release flow) |
| npm CLI | `packages/tiny-engineer-cli/package.json` → `"version"` | optional `cli-v0.1.0` |

Use **SemVer** only for the CLI surface users depend on:

| Bump | When |
|---|---|
| **patch** | bugfix, docs in the published package, no behavior change for hooks/setup |
| **minor** | new command/flag/IDE integration, backward-compatible |
| **major** | breaking: removed/renamed CLI flag, hook event id, or setup behavior that breaks existing hook configs |

CLI `0.1.0` need not match firmware `v0.2.0`. Do not bump npm because of CAD, PCB, or firmware-only changes.

Deprecated per-IDE packages (`tiny-engineer-cursor`, etc.) are **not** published to npm.

## Prerequisites

- npm account with 2FA; `npm login` / `npm whoami` locally
- Publish **only** from `packages/tiny-engineer-cli` (not the repo root `package.json`)
- Package name on npm is `tiny-engineer` (bin: `tiny-engineer`)

## Release checklist

1. **Land the CLI change on `main`** (or the branch you release from). Tests green:

   ```bash
   npm test --prefix packages/tiny-engineer-cli
   ```

2. **Bump version** in [`packages/tiny-engineer-cli/package.json`](../packages/tiny-engineer-cli/package.json) (and keep `package-lock.json` `name`/`version` in sync if present). Prefer a manual edit in this monorepo over `npm version` at the package root.

3. **Commit** (conventional commits, scope `integrations`), e.g.:

   ```text
   chore(integrations): release tiny-engineer 0.1.1
   ```

4. **Dry-run** (optional but recommended):

   ```bash
   cd packages/tiny-engineer-cli
   npm pack --dry-run
   npm publish --dry-run
   ```

   Expect a small tarball (`bin/`, `src/`, `LICENSE`, `README.md`, `package.json`) — no `test/`, no firmware/CAD.

5. **Publish**:

   ```bash
   cd packages/tiny-engineer-cli
   npm publish --access public
   ```

   Enter the npm OTP when prompted.

6. **Verify**:

   ```bash
   npm view tiny-engineer version
   npx -y tiny-engineer@0.1.1 --help   # use the version you just published
   ```

7. **Optional git tag** (docs / future CI only; not required for npm):

   ```bash
   git tag cli-v0.1.1
   git push origin cli-v0.1.1
   ```

   Do **not** use a bare `v*` tag for CLI — that triggers the firmware release workflow.

8. **Docs**: user install path is `npx -y tiny-engineer …` ([integration.md](integration.md)). Bump docs only if flags or setup behavior changed.

## What not to do

- Do not `npm publish` from the repository root (root wraps CLI + deprecated cursor bin for tarball `npx`).
- Do not bump or publish deprecated per-IDE packages.
- Do not expect a firmware `v*` GitHub Release to update npm.
- Do not invent a second public name; users run `npx tiny-engineer` / `tiny-engineer setup`.

## Later automation (optional)

A GitHub Action can publish on push of tags matching `cli-v*` using an npm Automation token (`NPM_TOKEN`). Until that exists, the checklist above is the full process.
