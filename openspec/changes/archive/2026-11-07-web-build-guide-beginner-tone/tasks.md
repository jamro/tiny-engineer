# Tasks

## 1. Shop copy and cart data

- [x] 1.1 Rewrite beginner-facing strings in `web/shop/cart.json` (board title/pathLabel/disclaimer, download labels, six JLCPCB steps with plain role words + optional Gerber/BOM/CPL parentheticals, lane ledes, item labels/blurbs, why-five servo notes) and verify JSON parses and download `url` fields still point at `fab/main-control-board/{gerbers.zip,bom.csv,positions.csv}`
- [x] 1.2 Add USB power next-action: set a recommended buy `url` and/or tips text for `power-usb` (no kit price) and verify the fasteners lane shows a Buy or tips control for that row in the shop UI
- [x] 1.3 Demote contributor crumbs (`cart.json` / `shopping.md` footer) behind Advanced (or equivalent) and verify a first-load shop view does not present raw data files as a primary CTA

## 2. Shop interaction and visuals

- [x] 2.1 Update `web/js/shop-cart.js` “Your build” / summary strip so it omits `parts/…` repository paths while still reflecting model, size class, and ×5 buy quantity; verify the strip text on SG90 and a compact preset
- [x] 2.2 Enlarge checklist check hit targets and clarify mark-obtained/ordered labeling; verify clicking the row check toggles state and persists across reload
- [x] 2.3 Adjust glance/timeline wording so servo preset selection reads as chosen, not purchased, while board/module/fastener progress tracks checks only; verify fresh load with default SG90 does not mark board/modules done
- [x] 2.4 Add non-blocking incomplete-checks warning near Continue to Print; verify navigation to `/print/` still works when checks are empty and the warning is absent when all default checks are set
- [x] 2.5 Diagnose and fix ES9051 (and any other) servo card image render so cards show asset or placeholder; verify FS0307 and ES9051 cards have no broken-image hole at `/shop/`
- [x] 2.6 Browser smoke on `/shop/`: servo pick, fab download labels, checklist toggle, Continue warn, USB next-action — verify against the new `web-shop` delta scenarios

## 3. Other build-stage chrome

- [x] 3.1 Soften Print, Flash, Assembly, and Setup stage titles/ledes/primary CTAs for solder-never voice without inlining Markdown bodies; verify each stage page lede reads as plain language and still links out where it did before
- [x] 3.2 Confirm shared timeline still lists Shop → Setup on those pages; verify current stage indication remains correct after copy edits

## 4. Integration check

- [x] 4.1 Walk Shop → Print once in the browser after changes and verify beginner chrome holds, shop deep fixes remain, and no kit price/checkout appears

## Workflow follow-up

- Archive the change after review when applying `/opsx-archive` (creates main `web-build-guide` spec and merges `web-shop` delta).
