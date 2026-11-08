# Tasks

## 1. Spike: headless fab export on CI image

- [x] 1.1 Prove Fabrication Toolkit CLI can run headless on Ubuntu with a pinned KiCad that provides `pcbnew` (or document failure) and verify it emits Gerber archive + `bom.csv` + `positions.csv` for `hardware/boards/main-control-board/`
- [x] 1.2 If Toolkit CLI is blocked, spike `kicad-cli` gerbers + BOM (LCSC fields) + pos with header remap and verify column names match JLCPCB-oriented Toolkit shape; record chosen path in the export script comments

## 2. Export script and output layout

- [x] 2.1 Add a repo script that exports main-control-board into `web/shop/fab/main-control-board/{gerbers.zip,bom.csv,positions.csv}` (stable public names) and exits non-zero on failure — verify local or CI dry-run creates all three files with non-empty content and BOM/CPL headers present
- [x] 2.2 Add `web/shop/fab/` to `.gitignore` (or equivalent) so generated outputs are not committed from local runs, and verify `git check-ignore` covers those paths while shop HTML/JSON remain tracked
- [x] 2.3 Document the one-liner to run the export script in `docs/hardware/order-main-control-board.md` alternate/self-export section (or script `--help`) and verify the documented command matches the script entrypoint

## 3. Pages pipeline

- [x] 3.1 Extend `.github/workflows/pages.yml` path filters to include `hardware/boards/main-control-board/**` and the new export script path(s), and verify the workflow YAML still parses (`actionlint` if available, else visual review of `on.push.paths`)
- [x] 3.2 Wire KiCad/Toolkit (or chosen fallback) install + cache + export script into the Pages job before `upload-pages-artifact`, and verify a workflow run (or act/dry equivalent) fails closed when the export step is forced to fail
- [x] 3.3 On a successful Pages build, verify the uploaded site artifact contains `/shop/fab/main-control-board/gerbers.zip`, `bom.csv`, and `positions.csv`

## 4. Shop UI

- [x] 4.1 Update `web/shop/cart.json` JLCPCB steps and add same-origin download URLs for the three fab files; verify JSON parses and URLs point under `../fab/main-control-board/` (or `/shop/fab/...` as used by the page)
- [x] 4.2 Update shop rendering (`web/js/shop-cart.js` / HTML) to show download controls for Gerber, BOM, and positions; verify the board lane no longer lists KiCad install as a primary prerequisite and still links the full guide
- [x] 4.3 Smoke-check shop page in browser or static serve: download links resolve relative to `/shop/` and the continue-to-print control still works

## 5. Docs alignment

- [x] 5.1 Rewrite `docs/hardware/order-main-control-board.md` to lead with site downloads (always-latest from Pages/`main`), keep KiCad + Fabrication Toolkit as alternate, and keep JLCPCB stock/orientation review; verify shopping/getting-started links to this guide still make sense
- [x] 5.2 Adjust any shop-facing copy in `docs/shopping.md` or related pointers that still claim KiCad export is the only way to get fab files, and verify wording matches the download-first path

## Workflow follow-up

- After a real JLCPCB upload/review (or full test order) of the generated package, file follow-ups for any BOM/CPL issues.
- Archive the change after review requirements are satisfied (`/opsx-archive`).
