# Design

## Context

See proposal.md for motivation. Today `web/shop/index.html` is a single CTA to GitHub `docs/shopping.md`. Sibling stages `/print/`, `/connect/`, `/setup/`, `/use/` are the same stub pattern; `/flash/` and `/assembly/` are interactive. Build chrome already exists: `build-timeline.js`, `build-progress.js` (`localStorage` stage + assembly step). Visual system: `web/STYLE.md`, `css/tokens.css`, `css/base.css` (`.panel`, `.btn`, dark palette). Buyer truth for parts remains `docs/shopping.md` and `docs/hardware/order-main-control-board.md`; fab outputs stay local (`production/` gitignored) — builders export via KiCad Fabrication Toolkit.

## Goals / Non-Goals

**Goals:**

- Make `/shop/` a useful stage-1 tool: decide servo, start JLCPCB both-sides PCBA, check off the rest of the cart, then continue to `/print/`.
- Keep presentation on-brand (engineered panels, not SaaS storefront) and visually clear with images (real or dummyimage placeholders).
- Single structured data file for the UI cart; persist progress in the browser.

**Non-Goals:**

- E-commerce, payments, kit SKUs, summed prices, availability scraping.
- In-page print/order-parts flow (belongs on `/print/`).
- Advanced breakout cart rows (docs link only).
- PCBA Top / Economic / PCB-only as shop choices.
- Auto-sync or codegen between `cart.json` and `shopping.md`.
- Shipping committed Gerber/BOM ZIPs on Pages (unless a later change adds Release assets).

## Decisions

### 1. Page composition (B + C + A)

| Layer | Role |
| --- | --- |
| **B — Decisions** | Servo picker: SG90 (default), FS0307, ES9051. HD-1370A omitted. |
| **C — Timeline** | NOW = JLCPCB board; NEXT = plug-in modules; ALSO = fasteners + power/cable. |
| **A — Body** | Checklist rows from `cart.json`, filtered/labeled by servo pick. |

Exit: primary CTA → `/print/` (hint print folder `parts/{servo_id}/`). Advanced breakouts: one muted line → GitHub `docs/shopping.md#advanced-extra-modules`.

**Alternatives considered:** Storefront-style product grid (rejected — landing non-goal). Doc-only stub (status quo — not useful). Full JLCPCB walkthrough copy-paste of the entire markdown page (too long; keep short steps + “full guide” link).

### 2. Data: `web/shop/cart.json`

- Fetched same-origin from the shop page (static Pages).
- Owns: servo options, lane grouping (`now` / `next` / `also`), item id/qty/label/notes/url/image, JLCPCB short-step copy (or a nested `jlcpcb` block), advanced-docs URL.
- Servo line qty comes from the selected servo entry (`qty: 5`); item may use `qtyFrom: "servo"`.
- **Manual sync** with `docs/shopping.md` when either changes. No CI gate required for this change.

**Alternatives considered:** Parse Markdown in the browser (fragile). Generate MD from JSON in this change (out of scope). Hardcoded HTML only (harder to maintain qty/links).

### 3. Persistence

Reuse the defensive `localStorage` pattern from `build-progress.js` (try/catch, ignore private mode).

| Key | Value |
| --- | --- |
| `tiny-engineer.shop.servo` | `sg90` \| `fs0307` \| `es9051` |
| `tiny-engineer.shop.checked` | JSON array/object of checked item ids |

Optional later: `/print/` reads `tiny-engineer.shop.servo` (not required to ship shop). Provide a small “Clear checks” control.

### 4. JLCPCB lane (in-page)

Fixed path: **both-sides PCBA**, Standard assembly language from the order doc. Short numbered steps on `/shop/`:

1. KiCad 10 + Fabrication Toolkit  
2. Export Gerber ZIP + `bom.csv` + `positions.csv`  
3. Upload Gerber at jlcpcb.com; enable PCB Assembly, **both sides** / Standard  
4. Upload BOM + CPL; review stock and orientation; pay  
5. On arrival: inspect → continue to `/connect/` (plug-in modules; no header soldering for both-sides)

Deep link: full guide `docs/hardware/order-main-control-board.md`. Disclaimer: no partnership; orders at own risk. Checklist items for board: e.g. `ordered` / `arrived` (or one item with note).

**Alternatives considered:** CTA-only to GitHub (rejected — user asked JLCPCB on `/shop`). Top vs Both radio (rejected — Both only). Host fab ZIP on site (blocked by current gitignore / no Release packaging in scope).

### 5. Visual layout

Follow STYLE: IBM Plex, `--panel` / `--accent`, `.panel` / `.panel-label` / `.btn`. Prefer modular panels over large rounded SaaS cards.

```
+------------------------------------------------------------------+
| build-timeline (shop current)                                    |
| eyebrow · h1 Shop · lede                                         |
+---------------------------+--------------------------------------+
| SERVO PICKER              | preview 640x480                      |
| [SG90*] [FS0307] [ES9051] | dummy or real photo                  |
| mono: parts/{id}/         |                                      |
+---------------------------+--------------------------------------+
| ORDER TIMELINE — NOW / NEXT / ALSO (compact strip or list)       |
+------------------------------------------------------------------+
| NOW — Main board (JLCPCB both)                                   |
| [board 480x360] | numbered steps + full-guide link + checks      |
+------------------------------------------------------------------+
| NEXT — Modules (row checklist)                                   |
| [ ] thumb 96x96 | name | qty | Buy link                          |
+------------------------------------------------------------------+
| ALSO — Fasteners / power / cable                                 |
+------------------------------------------------------------------+
| Advanced breakouts → GitHub (one line)                           |
| [Continue to Print]                                              |
+------------------------------------------------------------------+
```

**Images:** Prefer real assets when available (`web/media/`, board `*-preview.jpg` if appropriate to copy or hotlink carefully). Missing assets:

`https://dummyimage.com/{W}x{H}/191919/7cff6b&text={Label}`

Suggested sizes: preview 640×480, board 480×360, thumbs 96×96. Store URLs (or path + size) on each cart item / servo in JSON so HTML stays dumb.

**Motion (2–3):** servo switch updates preview + print-folder hint; checkbox toggles row `is-done`; subtle current-timeline emphasis. No accent flood / glass / purple gradients.

**Alternatives considered:** Image-free dense tables (weaker for makers). Ecommerce product cards with prices (forbidden claims).

### 6. Implementation shape

- Enrich `web/shop/index.html` structure (semantic sections, empty mounts).
- New script(s) under `web/js/` (e.g. `shop-cart.js`) — load JSON, render lanes, wire checks/servo, no framework (match flash/assembly vanilla style where possible).
- CSS: page-scoped rules under `.page-shop` in `base.css` (or small `shop.css` only if base would bloat — prefer extending base patterns).
- Keep header/timeline/footer mounting identical to other stages.

## Risks / Trade-offs

| Risk | Mitigation |
| --- | --- |
| `cart.json` drifts from `shopping.md` | Manual sync called out in tasks; keep JSON as UI source of truth for the page |
| Dummyimage dependency / offline | Placeholders only until assets land; prefer checking in real images later |
| JLCPCB UI labels change | Copy outcomes not button names; point to full doc |
| Builder lacks KiCad | Stated in steps; cannot one-click fab without a later Release ZIP change |
| Over-scoping print into shop | Hard boundary: no print wizard on this page |

## Migration Plan

- Deploy with normal GitHub Pages from `web/` — additive UI replace of stub.
- Rollback: revert shop HTML/JS/JSON/CSS; timeline still points at `/shop/`.
- No NVS/firmware migration.

## Open Questions

- Exact checkbox id set for JLCPCB (`board-ordered` vs split ordered/arrived) — pick at implement time without changing scope.
- Whether `/print/` should read `tiny-engineer.shop.servo` in this change or a follow-up — default: shop-only persist; print read optional stretch.
