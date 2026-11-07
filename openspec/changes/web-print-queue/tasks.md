# Tasks

## 1. Parts data

- [x] 1.1 Add `web/print/parts.json` with format version, groups (PartsSetA copper, PartsSetB black, white/LampDiffuser), testers (`ServoSizingTester`, `ScrewSizingTest`), stock parts with group/qty/role/files `{3mf,stl}`, and servo entries `sg90`/`fs0307`/`es9051` with `filesBase` — verify JSON parses and part→group matches assembly print groups (no PartsSet aggregates as checklist rows; `UpperArm` qty 2)
- [x] 1.2 Document storage keys in JSON or script constants: read `tiny-engineer.shop.servo`, write `tiny-engineer.print.checked` — verify key strings match Shop’s `SERVO_KEY` exactly

## 2. Mesh publish (CI + local)

- [x] 2.1 Gitignore generated `web/print/files/` mesh tree (keep directory placeholder or README if needed) — verify `git check-ignore` (or status) would not track copied `.3mf`/`.stl` under that path
- [x] 2.2 Add a small sync script (e.g. `scripts/web/sync-print-files.sh` or `.mjs`) that copies individual `.3mf`/`.stl` for sg90/fs0307/es9051 from `3d_models/parts/` into `web/print/files/{servo}/`, excluding `PartsSetA`/`PartsSetB` — verify script exits 0 and expected files like `Head.3mf` / `Head.stl` exist for each servo
- [x] 2.3 Wire that sync into `.github/workflows/pages.yml` before Pages artifact upload and fail if a `parts.json` referenced file is missing — verify workflow YAML references the script and a dry local run covers JSON file list

## 3. Print page structure and styles

- [x] 3.1 Replace stub body in `web/print/index.html` with mounts for variant banner, Print-first section, color lanes, progress/clear, Continue → `../connect/`, Advanced README link; add Three import map + module script entry like assembly — verify page loads header/timeline/footer and is not a lone GitHub README CTA
- [x] 3.2 Add `.page-print` layout CSS in `web/css/base.css` (panels, lane headers with swatch, checklist rows with preview slot + dual download buttons, done state) per `web/STYLE.md` and Shop patterns — verify desktop and narrow viewport remain readable without a new palette

## 4. Print client behavior

- [x] 4.1 Add `web/js/print-queue.js` (ES module) that fetches `parts.json`, resolves servo from Shop storage (default `sg90`; unknown → sg90 + note), renders read-only variant + Shop link, testers, and color lanes with checkboxes — verify changing Shop servo then reloading Print switches labels/`filesBase`; Print has no servo radio
- [x] 4.2 Wire dual same-origin download controls (`.3mf` and `.stl`) per tester/part using active `filesBase` — verify hrefs point under `files/{servo}/` and both formats appear on each row
- [x] 4.3 Persist checklist ids in `tiny-engineer.print.checked` with defensive try/catch; restore on load; Clear checks; show progress counts — verify reload keeps checks and Clear resets without touching Shop keys
- [x] 4.4 Implement live STL previews (Three `STLLoader`, group swatch materials, IntersectionObserver + shared/pooled renderer, non-crashing fallback) — verify a visible row shows a mesh when files are synced, and with WebGL disabled or 404 STL the checkbox/downloads still work

## 5. Docs pointer

- [x] 5.1 Add a short pointer from `3d_models/README.md` and/or `docs/getting-started.md` §Print to the live `/print/` page without removing the markdown print instructions — verify the linked path matches the published print stage

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive` or project archive flow).
- Keep `parts.json` color groups manually aligned with CAD / assembly `print_group` until a CI assert exists.
