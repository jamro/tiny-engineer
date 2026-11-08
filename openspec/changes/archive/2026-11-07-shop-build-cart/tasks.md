# Tasks

## 1. Cart data

- [x] 1.1 Add `web/shop/cart.json` with format version, servo options (`sg90` default, `fs0307`, `es9051`; no HD-1370A), lanes `now` / `next` / `also`, item fields (id, label, qty or qtyFrom, url, notes, image), nested JLCPCB short steps + full-guide URL, and `advancedBreakoutsDoc` GitHub link — verify JSON parses and matches default-cart qty from `docs/shopping.md` (manual sync note in a one-line comment/`about` field)
- [x] 1.2 Fill image URLs: real assets where available under `web/media/` or copied board preview; otherwise `https://dummyimage.com/{W}x{H}/191919/7cff6b&text=…` at 640×480 (servo), 480×360 (board), 96×96 (thumbs) — verify each URL resolves in a browser or returns 200

## 2. Shop page structure and styles

- [x] 2.1 Replace stub body in `web/shop/index.html` with mounts for servo picker + preview, order timeline, JLCPCB lane, checklist lanes, advanced-docs line, clear-checks control, and Continue to Print → `../print/` — verify page still loads header/timeline/footer and has no lone “Open shopping instructions” primary CTA as the only content
- [x] 2.2 Add `.page-shop` layout CSS in `web/css/base.css` (reuse `.panel` / `.btn` / tokens; servo+preview row, checklist rows with thumbs, done state) per `web/STYLE.md` — verify desktop and narrow viewport: no overlapping timeline, readable rows, no new accent/palette

## 3. Shop client behavior

- [x] 3.1 Add `web/js/shop-cart.js` (or equivalent) that fetches `cart.json`, renders servos/lanes/JLCPCB steps, updates servo preview + `parts/{id}/` hint, and wires Buy links — verify selecting each servo updates label/qty/preview without a full page reload
- [x] 3.2 Persist `tiny-engineer.shop.servo` and `tiny-engineer.shop.checked` via defensive `localStorage` (try/catch like `build-progress.js`); restore on load; Clear checks clears checked ids — verify reload keeps servo + checks, and Clear resets checks only
- [x] 3.3 Ensure advanced breakouts are docs-link only (no PCA9685 / MAX98357A / USB-C breakout checklist rows) and the page has no checkout or summed kit price — verify by inspecting rendered DOM / copy against `specs/web-shop/spec.md`

## 4. Docs pointer

- [x] 4.1 Add a short pointer from `docs/shopping.md` and/or `docs/getting-started.md` §1 Shop to the live `/shop/` page (GitHub Pages URL or relative site path as used elsewhere) without removing the markdown cart — verify the linked path matches the published shop stage

## Workflow follow-up

- Archive the change after implementation and review (`/opsx-archive` or project archive flow).
- Do not auto-generate `shopping.md` from `cart.json` in this change; keep manual sync when either side changes.
