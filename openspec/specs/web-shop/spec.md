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
The shop page MUST let the visitor select exactly one of Tower Pro SG90, Feetech FS0307, or EMAX ES9051. SG90 MUST be the default when no saved choice exists. The shop UI MUST NOT offer PowerHD HD-1370A as a selectable preset.

#### Scenario: First visit
- **WHEN** a visitor opens `/shop/` with no saved servo choice
- **THEN** SG90 is selected and the cart reflects five SG90-class servos

#### Scenario: Compact preset
- **WHEN** the visitor selects FS0307 or ES9051
- **THEN** the servo checklist line shows quantity five for that model and the page indicates the matching `parts/{servo_id}/` print folder

#### Scenario: Legacy preset hidden
- **WHEN** a visitor views the servo picker
- **THEN** HD-1370A is not listed as an option

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
