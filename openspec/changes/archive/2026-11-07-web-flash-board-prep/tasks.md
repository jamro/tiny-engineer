# Tasks

## 1. Flash page shell and styles

- [x] 1.1 Add a Prepare the board section above `.hero` in `web/flash/index.html` (3D stage host, step title/body, Prev/Next, step counter, status) and verify the section appears before the install panel in the DOM order
- [x] 1.2 Add Three import map + module script hook for flash prep on the flash page (same CDN pattern as `/assembly/`) and verify the page loads without console module-resolve errors when opened locally
- [x] 1.3 Style the prep layout in `web/css/base.css` (and tokens only if needed) to match existing panels / assembly stage density and verify desktop and narrow widths keep step controls usable without overlapping the canvas

## 2. Prep viewer module

- [x] 2.1 Create `web/js/flash-prep.js` that creates a stage via `web/js/assembly/stage.js`, loads `../assembly/models/sg90.glb`, keeps only `MainControlBoard` / `ESP32-C3-Zero` / `ESP32-pin-headers`, frames the board, and verify those three parts are visible and the desk/robot parts are not in the scene
- [x] 2.2 Implement step 1 seat animation (module + headers elevated on +Z, ease into seated pose on step 1 entry / replay) and verify Prev/Next land on step 1 vs 2 with matching copy
- [x] 2.3 Implement step 2 world→screen callouts distinguishing board USB-C (correct) vs module USB-C (incorrect) and verify both labels track orbit/resize without blocking install controls below
- [x] 2.4 On WebGL or GLB failure, collapse or hide the canvas, keep step text + nav, show a short status error, and verify `flash-status.js` still loads releases and can show the install button

## 3. Copy and install panel cleanup

- [x] 3.1 Write beginner-plain step 1/2 titles and bodies (seat + USB mark; board USB not module) without a header-soldering step and verify wording leads with the job before optional chip names
- [x] 3.2 Shorten install panel-foot callouts that duplicate prep seating/USB guidance while keeping Chrome/Edge and data-cable guidance near install, and verify Connect & install plus the image-map table still update from the release picker

## 4. Docs pointer (light)

- [x] 4.1 Add a brief pointer in `docs/flash.md` (and getting-started flash bullet if needed) that the web flash page shows visual board prep first, and verify the link/path to `/flash/` remains correct

## Workflow follow-up

- Archive the change after review when applying `/opsx-archive` (creates main `web-flash` spec).
