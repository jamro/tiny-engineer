# Proposal

## Why

The public GitHub Pages build guide (`/shop/` → `/use/`) still reads like an engineer shopping list. Solder-never builders hit jargon (PCBA, Gerber, BOM, CPL, long pole, chip IDs), weak checklist affordances, a broken compact-servo image, and a power item with no buy path. We need beginner-first tone locked on the **web** guide — without rewriting repo Markdown docs.

## What Changes

- Add a shared beginner-voice contract for the seven web build stages (Shop → Print → Connect → Flash → Assembly → Setup → Use).
- Deepen `/shop/`: plain-language labels and blurbs, keep the six JLCPCB steps with friendlier wording, light servo framing (model titles stay; stronger size + why ×5; no `parts/…` path in the human “Your build” strip), clearer check UX (chosen ≠ ordered), soft warn on Continue when checks incomplete, fix EMAX image display, add USB power buy/tips despite no single SKU.
- Light chrome polish on the other six stages (H1/lede/panel labels/CTAs) so tone matches; deep content that lives in linked `docs/*.md` stays as-is.
- **Not** in scope: kit price/checkout (still forbidden), MD tone rewrite, redesigning thin stages into full in-page wizards, rewriting assembly viewer mechanics.

## Capabilities

### New Capabilities

- `web-build-guide`: Shared beginner-voice and stage-chrome rules for the public Pages build flow (Shop → Use), excluding repo Markdown documentation tone.

### Modified Capabilities

- `web-shop`: Beginner-facing shop copy/hierarchy, checklist affordances, fab download labels, USB buy/tips, and servo “Your build” presentation — without changing cart data source, servo preset set, no-kit-storefront rules, or fab file identities.

## Impact

- `web/shop/cart.json`, `web/js/shop-cart.js`, `web/shop/index.html`, shop CSS as needed.
- Stage HTML under `web/{print,connect,flash,assembly,setup,use}/` (and shared build chrome if copy lives there).
- Possibly `web/media/shop/` if EMAX image path/render needs a fix beyond HTML.
- OpenSpec: new `web-build-guide` main spec on archive; delta into `web-shop`.
- Out of scope: `docs/**` voice rewrite; `pcb-fab-package` generation; firmware; selling kits.
