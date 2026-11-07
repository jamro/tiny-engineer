# Proposal

## Why

`/shop/` is a thin stub that deep-links to GitHub `docs/shopping.md`. Builders leave the site to assemble a multi-vendor cart, miss order order (PCB fab is the long pole), and get no visual guidance. The build timeline already treats Shop as stage 1; flash and assembly are real tools — shop should help buy and sequence parts the same way, without becoming a kit storefront.

## What Changes

- Replace the `/shop/` stub with a build-facilitation page: servo decision, order timeline, JLCPCB both-sides PCBA short flow, and an interactive checklist.
- Drive the checklist from `web/shop/cart.json` (manual sync with `docs/shopping.md`).
- Persist servo choice and checked items in `localStorage`.
- Support servo presets SG90 (default), FS0307, ES9051; hide HD-1370A on the shop UI.
- First-class electronics path: order the main control board via JLCPCB (both-sides PCBA) in-page; advanced breakouts = single link to GitHub docs only.
- Print path stays on `/print/` — shop only exits toward it (and may hint `parts/{servo_id}/`).
- Visual layout using existing `web/` STYLE panels/tokens; placeholder images via dummyimage.com until real assets exist.
- **Non-goals:** kit storefront, checkout, fixed/summed BOM price, assembled-board SKU, affiliate catalog, auto-generating `shopping.md` from JSON, Top/Economic/PCB-only PCBA in the shop UI, hosting committed Gerber/BOM downloads.

## Capabilities

### New Capabilities

- `web-shop`: Public GitHub Pages shop stage — structured cart, servo selection, JLCPCB order coaching, checklist persistence, and visual presentation that help builders complete the “cart in hand” exit criterion before print.

### Modified Capabilities

- (none)

## Impact

- `web/shop/` (HTML/CSS/JS + `cart.json`)
- Shared build helpers if needed (`web/js/build-progress.js` patterns / new `web/js/shop*.js`)
- Possibly light CSS in `web/css/base.css` / tokens — no new visual system
- Docs: manual keep `docs/shopping.md` aligned with `cart.json`; optional one-line pointer from shopping/getting-started to `/shop/`
- No firmware, HTTP API, or package changes
