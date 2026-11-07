# Spec Delta

## MODIFIED Requirements

### Requirement: Servo preset selection
The shop page MUST let the visitor select exactly one of Tower Pro SG90, Feetech FS0307, or EMAX ES9051. SG90 MUST be the default when no saved choice exists. The shop UI MUST NOT offer PowerHD HD-1370A as a selectable preset. Human-facing selection summary MUST name the chosen model, robot size class, and buy quantity without requiring a repository path such as `parts/{servo_id}/` in the primary summary strip.

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

## ADDED Requirements

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
