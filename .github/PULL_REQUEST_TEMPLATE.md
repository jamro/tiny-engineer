<!-- Title: type(scope): summary — see CONTRIBUTING.md (feat/fix → SemVer; ! or BREAKING CHANGE: → MAJOR). -->

## What

## Checks

- [ ] Title is `type(scope): summary`
- [ ] Breaking HTTP / pins / NVS / servo defaults / hook CLI called out as `type!:` plus a `BREAKING CHANGE:` footer
- [ ] Firmware: `pio run` and/or `pio test -e native` when that code changed
- [ ] Packages: `npm test --prefix packages/tiny-engineer-cursor` and/or `…/tiny-engineer-antigravity` when that package changed
- [ ] HTTP: HTML index (`src/http/index_page.cpp`) + `docs/api.md` (and README / `docs/hardware/testing.md` if the route list changed)
- [ ] Settings: layers in `docs/settings.md`; no raw `access_token` in logs
- [ ] CAD: `.f3d` + exported `.3mf`; CERN-OHL-S; `AiEmblem.3mf` not used as a branding swap
- [ ] Pins: `include/pins.h` + `docs/hardware/` together
- [ ] Servo ranges not widened without a real robot
- [ ] Hardware tested: robot / bench / N/A
- [ ] No `.env`, tokens, or Wi-Fi passwords in logs or screenshots

## PCB (if this PR changes a board)

Full list: [docs/pcb.md](../docs/pcb.md#checklist)

- [ ] Board in `hardware/boards/<name>/`; directory matches KiCad project name; KiCad 10
- [ ] Custom libs/3D in the board directory; project-relative paths (`${KIPRJMOD}`); no absolute local paths
- [ ] Source only — no backups, autosave, `*.kicad_prl`, `fp-info-cache`, Gerbers, drill, PDF, BOM, or PnP
- [ ] ERC and DRC reviewed; no unexplained suppressions
- [ ] Board `README.md` updated (purpose, status, interfaces, assumptions, built/tested)
- [ ] New path annotated CERN-OHL-S-2.0 in `REUSE.toml`
- [ ] PR states: schematic changed? layout changed? symbols/footprints changed?
- [ ] Physical status: KiCad review only / manufactured / manufactured and tested
