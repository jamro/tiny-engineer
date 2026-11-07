# Design

## Context

See proposal.md — Why. Shop UI is data-driven from `web/shop/cart.json` rendered by `web/js/shop-cart.js`. Other build stages are mostly thin HTML shells with shared timeline (`web/js/build-timeline.js`). Existing `web-shop` forbids kit price/checkout; `pcb-fab-package` already publishes Gerber/BOM/positions under `/shop/fab/`. EMAX asset `web/media/shop/es9051.jpg` is a valid JPEG on disk — browser “broken” state is a render/path/CSS issue to diagnose, not a missing file.

## Goals / Non-Goals

**Goals:**

- Centralize beginner voice rules in capability `web-build-guide`; deepen shop via `cart.json` + shop JS/CSS; light-touch stage chrome elsewhere.
- Keep six JLCPPCB steps; only reword and relabel.
- Preserve servo model titles (light framing); drop repo paths from human “Your build” strip.
- Improve check hit targets and chosen≠ordered messaging; soft warn on Continue; USB next-action; fix servo card images.

**Non-Goals:**

- Rewriting `docs/**` tone.
- Turning Print/Flash/… into full in-page manuals.
- Changing fab file identities or JLCPCB both-sides-only policy.
- Adding kit totals or checkout.

## Decisions

### 1. Copy-first in `cart.json`, chrome in stage HTML

Prefer updating `label` / `blurb` / `pathLabel` / `downloads[].label` / `steps[]` / `lede` / `title` in `cart.json`, and stage `index.html` ledes/CTAs, over large JS rewrites.

**Alternatives considered:** Hard-code beginner strings only in JS (splits source of truth from cart). Rejected.

### 2. Plain labels with stable download URLs

Visible fab chip text becomes role language (e.g. board layout / parts list / placement). `url` fields stay `fab/main-control-board/{gerbers.zip,bom.csv,positions.csv}`. Step text may say “board layout file (Gerber)” once so JLCPCB UI still matches.

**Alternatives considered:** Rename published files on disk. Rejected — breaks `pcb-fab-package` and bookmarks.

### 3. Light servo framing

Keep card titles as model names. Strengthen path headers / helper copy for size and “why ×5”. Change `renderYourBuild` (or equivalent) so `printFolder` is not shown as primary human text; print stage can still consume servo id from storage.

**Alternatives considered:** Heavy size-as-title cards. Deferred per exploration.

### 4. Checklist UX

Enlarge operable check target (label wraps row media/check). Timeline glance: servo step = “chosen”; board/modules/fasteners = checked counts only. Soft warn near Continue when `checked` incomplete — do not disable the link.

**Alternatives considered:** Hard-gate Continue. Rejected — builders may print while board fabs.

### 5. USB power next-action

`power-usb.url` is null today. Prefer a curated example buy URL for a known-good ≥2 A supply and/or data cable **or** a tips disclosure if no durable SKU. No price sum. Record chosen approach in tasks when picking the concrete link.

**Alternatives considered:** Leave null forever. Rejected for solder-never UX.

### 6. Shared tone vs deep shop

`web-build-guide` owns cross-stage voice + timeline. Shop-specific behavior stays in `web-shop` delta. Non-shop stages: edit HTML ledes/labels only unless a shared string helper already exists.

### 7. Contributor crumbs

Hide or demote `cart.json` / `shopping.md` footer links behind an Advanced disclosure already used for breakouts — keeps beginners from raw data files.

## Risks / Trade-offs

- [Plain labels vs JLCPCB UI jargon] → Mitigation: keep parenthetical Gerber/BOM/CPL once in steps; full guide link stays.
- [USB SKU rot] → Mitigation: tips fallback text always present; link optional and documented in cart notes.
- [Image “fix” is CSS/cache] → Mitigation: reproduce in browser; fix `src`/CSS/`onerror` placeholder; do not replace valid JPEG without cause.
- [Tone drift on thin stages that deep-link MD] → Accepted: MD stays technical; chrome only.

## Migration Plan

- Static Pages deploy: no firmware/NVS migration.
- `localStorage` keys for servo/checks unchanged unless rename required (avoid rename).
- Rollback = revert web copy/JS/CSS; specs archive separately.

## Open Questions

- Exact USB buy URL vs tips-only — pick during apply against current retailers; tips text mandatory either way.
