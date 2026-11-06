# Tasks

## 1. Step data and validation

- [x] 1.1 Add optional `center` boolean handling in `web/js/assembly/steps.js` `validate()` (reject non-boolean when present) and document the field in `docs/3d/assembly-viewer.md` Steps; verify with a unit case in `scripts/assembly/test_viewer.mjs` that bad `center` fails and good data still validates
- [x] 1.2 Insert a `bench-window` section in `web/assembly/steps.json` after `hat` / before `neck` (`doc: "#2a-bench-window-centering"`, empty `add`, `center: true`), mark centering on Neck center, Head–Neck fit, body servo center, and arm mount steps per design.md, trim duplicated full-recipe prose from those `text`s, and leave chest pocket seats unmarked; verify `node scripts/assembly/test_viewer.mjs` and `node scripts/assembly/check_steps.mjs` (or the project’s usual step check path) pass and a coverage assertion lists the expected marked step titles

## 2. Build card callout UI

- [x] 2.1 Add a centering callout element to `web/assembly/index.html` / CSS (`asm-center-callout` or reuse `.callout`) and wire `build.js` to show shared §2a procedure copy (AP → Assembly support → Move all to 90°, optional Identify, unplug, do not finish wizard) when `step.center` is true and hide it otherwise; verify manually or via a small DOM/logic assertion that marked vs unmarked steps toggle visibility and Back/Play/Next stay above the changing content
- [x] 2.2 Confirm callout wording matches `docs/3d/assembly.md` §2a (link to that anchor from the callout or section doc link) and that no joint-pose control was added; verify by reading the rendered copy against the guide and checking the fixed-pose chrome is unchanged

## 3. Integration check

- [x] 3.1 Run `node scripts/assembly/test_viewer.mjs` (and clipping/step checks if step order changes affect them) and spot-check the build player through Bench window → Neck → Head join → Belly → Arms; verify tests pass and each centering step shows the callout while pocket-only chest seats do not

## Workflow follow-up

- Archive the change after review when implementation is done (`/opsx-archive` or project archive flow).
