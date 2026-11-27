# web-shop Specification

## Purpose

Helps builders complete the shop stage of the public GitHub Pages build flow: buy the assembled main control board and check off the default cart (including servo model) before print — without selling a kit.

## Requirements

### Requirement: Shop stage presents a build cart
The `/shop/` page MUST present an in-page build cart for the default Tiny Engineer parts path. It MUST NOT be only a single external link to the shopping markdown document.

#### Scenario: Open shop
- **WHEN** a visitor opens `/shop/` on the published site
- **THEN** the page shows an order timeline, an assembled-board buy card first (with board-ordered check on that card), and checklist lanes for plug-in modules and fasteners
- **AND** the page does not present a separate top-level “pick a servo” step before the board
- **AND** the page does not show a second standalone “board ordered” checklist panel under the board card

### Requirement: Servo preset selection
The shop page MUST let the visitor select exactly one of Tower Pro SG90, Feetech FS0307, or EMAX ES9051 from the plug-in modules servo checklist row (not a separate shop step). SG90 MUST be the default when no saved choice exists. The shop UI MUST NOT offer PowerHD HD-1370A as a selectable preset. The servo row MUST name the chosen model, give a sense of robot size (approximate assembled dimensions and/or a plain size label such as standard/compact), and name the buy quantity, without requiring a repository path such as `parts/{servo_id}/` in the primary human-facing copy.

#### Scenario: First visit
- **WHEN** a visitor opens `/shop/` with no saved servo choice
- **THEN** SG90 is selected on the plug-in modules servo row and the cart reflects five SG90-class servos

#### Scenario: Compact preset
- **WHEN** the visitor selects FS0307 or ES9051 on the servo row model control
- **THEN** the servo checklist line shows quantity five for that model and indicates standard vs compact / robot size without requiring a `parts/…` repository path in the primary human-facing copy

#### Scenario: Legacy preset hidden
- **WHEN** a visitor views the servo model control on the plug-in modules row
- **THEN** HD-1370A is not listed as an option

#### Scenario: Standard is the default path
- **WHEN** a visitor views the servo row with no saved choice or with SG90 saved
- **THEN** SG90 remains the selected standard build option

### Requirement: Beginner-plain shop labels
Primary shop titles, lane ledes, checklist item names shown as the main label, and the board-card path badge MUST use plain language that states the job or outcome first. Vendor names and chip IDs MAY appear as secondary text.

#### Scenario: Board lane title is plain
- **WHEN** a visitor views the assembled-board buy card
- **THEN** the primary heading describes the assembled main control board in plain words

#### Scenario: Module labels lead with role
- **WHEN** a visitor views plug-in module checklist rows
- **THEN** each row’s primary label or adjacent plain blurb makes the part’s job clear (for example brain module, face screen, speaker) without relying only on chip identifiers

### Requirement: Explain why five identical servos
The plug-in modules servo row MUST tell visitors they need five identical units of one model and MUST NOT encourage mixing models or sizes.

#### Scenario: Quantity rationale visible
- **WHEN** a visitor views the servo checklist line
- **THEN** the page states that five identical servos are required and mixing models is not allowed

### Requirement: Chosen versus ordered checklist language
Servo preset selection MUST NOT be presented as the same progress state as marking the board or modules ordered. Checklist progress for buyable or orderable items MUST reflect explicit visitor checks. Check controls MUST be operable from the checklist row with a hit target larger than the decorative mark alone, and the control label MUST make clear the visitor is marking the item obtained or ordered.

#### Scenario: Default servo is chosen not purchased
- **WHEN** a visitor first lands on `/shop/` with SG90 selected and no checklist checks saved
- **THEN** timeline or glance copy does not claim board/modules/fasteners are done solely because a servo preset is selected

#### Scenario: Row marks ordered
- **WHEN** a visitor activates the check control on a checklist item row
- **THEN** that item is marked checked, the state persists per existing checklist persistence rules, and the control remains reachable from the row

### Requirement: Soft warn before continue with incomplete checks
The continue-to-print control MUST remain available. When required checklist progress is incomplete, the shop page MUST show a non-blocking warning that naming remaining work (especially the board order) before or beside that control. The page MUST NOT block navigation to `/print/`.

#### Scenario: Incomplete checks still allow print
- **WHEN** a visitor activates continue-to-print with unchecked board or module items
- **THEN** they can still navigate to `/print/` and see a non-blocking reminder that shopping checks remain incomplete

#### Scenario: Complete checks omit warn
- **WHEN** a visitor has checked all default-cart checklist items and continues to print
- **THEN** the shop page does not show the incomplete-checks warning

### Requirement: USB power buy or tips path
The USB supply and data-cable checklist item MUST offer either a recommended external buy link, or an in-page tips control that explains how to choose any ≥2 A supply and a data-capable USB-C cable. The shop MUST NOT invent a fake single kit SKU or summed price for that item.

#### Scenario: Power item has a next action
- **WHEN** a visitor views the fasteners-and-power lane
- **THEN** the USB supply and data-cable row exposes a Buy link or a clear tips action for choosing compatible power and cable

### Requirement: Servo preview images render
The plug-in modules servo row MUST show a working image for the selected preset (real asset or placeholder). A missing or failed image MUST NOT leave a blank broken-image hole without a placeholder fallback.

#### Scenario: Compact presets show images
- **WHEN** a visitor views FS0307 and ES9051 cards
- **THEN** each card displays an image or an intentional placeholder, not a broken-image icon with empty content

### Requirement: Cart data from structured JSON
The shop checklist and related shop UI data MUST load from a same-origin `cart.json` (or equivalent structured cart file under `web/shop/`). The page MUST NOT scrape `docs/shopping.md` at runtime to build the cart.

#### Scenario: Cart loads
- **WHEN** the shop page initializes successfully
- **THEN** checklist lanes and items are rendered from the structured cart file

### Requirement: Order timeline lanes
The shop page MUST present the default cart in an order that prioritizes the main control board first, then plug-in modules, then fasteners and power/cables.

#### Scenario: Timeline order
- **WHEN** a visitor views the shop body
- **THEN** the assembled-board buy card appears before module lines and before fasteners and power lines

### Requirement: Assembled board buy card
The shop page MUST present the main control board as an external buy link to the Elecrow assembled-board product. The card MUST state that plug-in modules are separate. The shop UI MUST NOT present Gerber, BOM, or placement downloads, or a JLCPCB upload walkthrough, on the board card, and MUST NOT tell the visitor that fab files are unnecessary or that the board is sold one at a time. A quiet link to the self-order guide MAY sit under the board card for people changing the open-source design. That link MUST NOT be a primary board step.

#### Scenario: Board card is a buy link
- **WHEN** a visitor views the board card
- **THEN** they see one primary control that opens the Elecrow assembled-board product, and a way to mark the board ordered

#### Scenario: No fab walkthrough on the card
- **WHEN** a visitor views the board card
- **THEN** the card does not list Gerber, BOM, or placement downloads and does not list JLCPCB upload steps

### Requirement: Self-order guide stays off the primary path
The shop page MAY link the self-order guide in quiet text under the board card, for people who change the open-source design and order the board themselves. That link MUST NOT replace the Elecrow buy control as the default board step.

#### Scenario: Quiet self-order link
- **WHEN** a visitor views the board section
- **THEN** a low-emphasis link to the order-main-control-board documentation is present, and the Elecrow buy control remains the primary action

### Requirement: Advanced breakouts docs-only
The shop page MUST NOT list PCA9685, MAX98357A, or USB-C breakout boards as buyable checklist rows for the default path. It MUST provide a single link to the GitHub shopping documentation section for advanced extra modules.

#### Scenario: Advanced path
- **WHEN** a visitor needs separate breakout modules instead of the main control board
- **THEN** the shop page points them to the advanced modules documentation link and does not expand those parts into the default checklist

### Requirement: Checklist persistence
The shop page MUST persist the selected servo preset and which checklist items are checked in browser `localStorage` (or equivalent browser storage). Reloading the page MUST restore that state when storage is available.

#### Scenario: Reload keeps progress
- **WHEN** a visitor selects a servo, checks one or more items, and reloads `/shop/`
- **THEN** the same servo and checked items remain selected

#### Scenario: Storage unavailable
- **WHEN** browser storage throws or is unavailable
- **THEN** the shop page still renders and remains usable without crashing

### Requirement: No kit storefront claims
The shop page MUST NOT present a checkout, a fixed or summed kit/BOM price, or the Elecrow board as a complete robot kit. An external link to that single assembled board is allowed.

#### Scenario: No price or checkout
- **WHEN** a visitor views `/shop/`
- **THEN** there is no checkout control and no total kit price claim

### Requirement: Visual cart presentation
The shop page MUST use the site's existing visual system (panels, tokens, typography) and MUST show images for the servo preview and cart items, using placeholder images when real assets are not available.

#### Scenario: Images present
- **WHEN** a visitor views the servo area and checklist rows
- **THEN** each has an image (real asset or placeholder) sized for that slot

### Requirement: Continue to print stage
The shop page MUST provide a clear control that navigates to the `/print/` build stage. The shop page MUST NOT host the home-print or print-service order wizard.

#### Scenario: Exit to print
- **WHEN** a visitor activates the continue-to-print control
- **THEN** they navigate to `/print/`
