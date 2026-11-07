# web-shop Specification

## Purpose

Helps builders complete the shop stage of the public GitHub Pages build flow: choose a servo preset, order the main control board through JLCPCB, and check off the default cart before print — without selling a kit.

## Requirements

### Requirement: Shop stage presents a build cart
The `/shop/` page MUST present an in-page build cart for the default Tiny Engineer parts path. It MUST NOT be only a single external link to the shopping markdown document.

#### Scenario: Open shop
- **WHEN** a visitor opens `/shop/` on the published site
- **THEN** the page shows servo selection, an order timeline, a JLCPCB board lane, and a checklist of default-cart items

### Requirement: Servo preset selection
The shop page MUST let the visitor select exactly one of Tower Pro SG90, Feetech FS0307, or EMAX ES9051. SG90 MUST be the default when no saved choice exists. The shop UI MUST NOT offer PowerHD HD-1370A as a selectable preset. Human-facing selection summary MUST name the chosen model, give a sense of robot size (approximate assembled dimensions and/or a plain size label such as larger/compact), and name the buy quantity, without requiring a repository path such as `parts/{servo_id}/` in the primary summary strip.

#### Scenario: First visit
- **WHEN** a visitor opens `/shop/` with no saved servo choice
- **THEN** SG90 is selected and the cart reflects five SG90-class servos

#### Scenario: Compact preset
- **WHEN** the visitor selects FS0307 or ES9051
- **THEN** the servo checklist line shows quantity five for that model and the page indicates the matching print parts set for that servo without requiring a `parts/…` repository path in the primary human-facing summary

#### Scenario: Legacy preset hidden
- **WHEN** a visitor views the servo picker
- **THEN** HD-1370A is not listed as an option

#### Scenario: Recommended path marked for beginners
- **WHEN** a visitor views the servo picker with SG90 as the default or selected recommended path
- **THEN** SG90 is visually marked as the starting / recommended choice for beginners

### Requirement: Beginner-plain shop labels
Primary shop titles, lane ledes, checklist item names shown as the main label, JLCPCB path badge, and fab download control labels MUST use plain language that states the job or outcome first. Vendor names, chip IDs, and fab file type names MAY appear as secondary text. The same-origin download targets MUST remain the published Gerber ZIP, `bom.csv`, and `positions.csv`.

#### Scenario: Board lane title is plain
- **WHEN** a visitor views the JLCPCB board lane
- **THEN** the primary heading describes the main assembled control board in plain words before or instead of leading with “JLCPCB both-sides PCBA” alone

#### Scenario: Fab chips labeled for newcomers
- **WHEN** a visitor views fab download controls
- **THEN** each control’s visible label explains the file’s role in everyday words while still downloading the correct Gerber ZIP, BOM, or positions file

#### Scenario: Module labels lead with role
- **WHEN** a visitor views plug-in module checklist rows
- **THEN** each row’s primary label or adjacent plain blurb makes the part’s job clear (for example brain module, face screen, speaker) without relying only on chip identifiers

### Requirement: Explain why five identical servos
The shop servo step MUST tell visitors they need five identical units of one model and MUST NOT encourage mixing models or sizes.

#### Scenario: Quantity rationale visible
- **WHEN** a visitor views the servo picker or the servo checklist line
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
Each selectable servo preset card MUST show a working image for that preset (real asset or placeholder). A missing or failed image MUST NOT leave a blank broken-image hole without a placeholder fallback.

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
- **THEN** the board / JLCPCB lane appears before module lines and before fasteners and power lines

### Requirement: JLCPCB both-sides board lane
The shop page MUST include an in-page short order flow for the main control board via JLCPCB with both-sides PCBA as the supported path. The primary short steps MUST start from same-origin downloads of the published Gerber ZIP, `bom.csv`, and `positions.csv` (no KiCad install required for that path). It MUST link to the full order guide in the documentation. The shop UI MUST NOT present Top-only, Economic-only, or PCB-only as selectable shop order paths. The short steps MUST still tell the visitor to review stock and orientation at JLCPCB before paying.

#### Scenario: Board lane content
- **WHEN** a visitor views the board lane
- **THEN** they see numbered short steps that begin with downloading fab files from the site, then placing a both-sides JLCPCB PCBA order with stock/orientation review, plus a link to the full order-main-control-board guide

#### Scenario: Fab download controls
- **WHEN** a visitor views the board lane on a successfully deployed site
- **THEN** the lane exposes working same-origin links (or equivalent download controls) for the Gerber ZIP, `bom.csv`, and `positions.csv`

#### Scenario: No KiCad prerequisite in primary steps
- **WHEN** a visitor reads the in-page short JLCPCB steps
- **THEN** those steps do not require installing KiCad or Fabrication Toolkit as a prerequisite for the primary order path

### Requirement: Full guide remains available for self-export
The shop page MUST keep a link to the full order guide so builders who prefer (or need) local KiCad export can follow that path. The shop MUST NOT remove documentation access solely because downloads exist.

#### Scenario: Full guide link present
- **WHEN** a visitor views the JLCPCB board lane
- **THEN** a link to the full order-main-control-board documentation remains available

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
The shop page MUST NOT present a checkout, a fixed or summed kit/BOM price, or a ready-to-buy assembled main board as a product SKU.

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
