# Spec Delta

## Purpose

Helps builders complete the print stage of the public GitHub Pages build flow: see the parts set for the Shop-selected servo, track print progress by color group, preview meshes live, and download individual `.3mf` / `.stl` files — without hosting a print-service order wizard.

## ADDED Requirements

### Requirement: Print stage presents a part queue
The `/print/` page MUST present an in-page printable-parts queue for the selected servo preset. It MUST NOT be only a single external link to `3d_models/README.md`.

#### Scenario: Open print
- **WHEN** a visitor opens `/print/` on the published site
- **THEN** the page shows the locked servo variant, a Print-first tester section, color-grouped part rows with progress controls, and download controls

### Requirement: Servo variant locked from Shop
The print page MUST resolve the servo preset from the Shop browser storage key used by `/shop/` (`tiny-engineer.shop.servo`). Supported presets MUST be SG90, FS0307, and ES9051. When no saved choice exists, SG90 MUST be used. The print UI MUST NOT offer an in-page servo picker; it MUST show the active variant and a control that navigates to `/shop/` to change it. The print UI MUST NOT offer HD-1370A.

#### Scenario: Variant from Shop
- **WHEN** Shop storage has `fs0307` selected and the visitor opens `/print/`
- **THEN** the page identifies Feetech FS0307 (or equivalent label) as the active variant and file downloads resolve under that servo’s published files folder

#### Scenario: Default when unset
- **WHEN** a visitor opens `/print/` with no saved Shop servo choice
- **THEN** the page uses SG90 and still renders the queue

#### Scenario: Change requires Shop
- **WHEN** the visitor wants a different servo preset
- **THEN** the only in-page change path is a navigation control to `/shop/` (no radio/select on Print)

### Requirement: Part data from structured JSON
The print queue and related print UI data MUST load from a same-origin `parts.json` (or equivalent structured file under `web/print/`). The page MUST NOT scrape `3d_models/README.md` at runtime to build the list.

#### Scenario: Parts load
- **WHEN** the print page initializes successfully
- **THEN** tester rows and color-grouped part rows are rendered from the structured parts file

### Requirement: Color-grouped part lanes
The print page MUST group stock printable parts into color lanes that match the design print aggregates: PartsSetA (copper), PartsSetB (black), and LampDiffuser (white / translucent) as its own lane or equivalent solo group. Aggregate archive files `PartsSetA` / `PartsSetB` MUST NOT appear as normal home-print checklist rows.

#### Scenario: Lane order
- **WHEN** a visitor views the print body
- **THEN** copper, black, and white/translucent groups are visibly separated and parts appear only in their design color group

### Requirement: Print-first tester section
The print page MUST include a separate Print-first section for `ServoSizingTester` and `ScrewSizingTest` with checkboxes independent of the color-lane rows. That section MUST explain that testers come before the full set for home printing.

#### Scenario: Testers before lanes
- **WHEN** a visitor views `/print/`
- **THEN** the tester section appears above the color-grouped part lanes and each tester has its own checkbox and downloads

### Requirement: Progress checklist persistence
The print page MUST persist which tester and part checklist items are checked in browser `localStorage` (or equivalent), using a key distinct from Shop checklist storage. Reloading MUST restore that state when storage is available. Storage failures MUST NOT crash the page.

#### Scenario: Reload keeps print progress
- **WHEN** a visitor checks one or more print items and reloads `/print/`
- **THEN** those items remain checked

#### Scenario: Storage unavailable
- **WHEN** browser storage throws or is unavailable
- **THEN** the print page still renders and remains usable without crashing

### Requirement: Dual-format per-part downloads
Each listed tester and stock part row MUST provide same-origin download controls for both the `.3mf` and `.stl` mesh files for the active servo preset. Downloads MUST NOT require leaving the site to GitHub blob/raw as the primary path.

#### Scenario: Download both formats
- **WHEN** a visitor activates the `.3mf` or `.stl` control on a part row for the active servo
- **THEN** the browser retrieves that file from a same-origin URL under the published print files tree for that servo

### Requirement: Live STL previews
Each listed tester and stock part row MUST show a live 3D preview of that part loaded from its same-origin `.stl` for the active servo. Static PNG/JPEG thumbnails MUST NOT be the primary preview. If WebGL or mesh load fails, the row MUST remain usable (downloads and checkbox still work) with a non-crashing fallback.

#### Scenario: Preview from STL
- **WHEN** a visitor views a part row and WebGL is available
- **THEN** a live mesh preview for that part’s STL is shown in the row

#### Scenario: Preview failure
- **WHEN** WebGL is unavailable or the STL fails to load
- **THEN** the row still exposes checkbox and download controls without crashing the page

### Requirement: Published mesh files on Pages
The deployed site MUST publish individual `.3mf` and `.stl` files for SG90, FS0307, and ES9051 under a same-origin print files path used by the page. Those meshes MUST NOT be required as committed bulk under `web/print/files/` in git; publish MAY happen at Pages build/deploy time from `3d_models/parts/`.

#### Scenario: Deployed downloads resolve
- **WHEN** the Pages workflow has deployed successfully
- **THEN** same-origin URLs for individual part `.3mf` and `.stl` files for each supported servo return the mesh files

### Requirement: Visual system and continue
The print page MUST use the site’s existing visual system (panels, tokens, typography) consistent with `/shop/`. It MUST provide a clear control that navigates to the next build stage (`/connect/`). The print page MUST NOT host a third-party print-service order wizard for aggregated PartsSet uploads.

#### Scenario: Exit to connect
- **WHEN** a visitor activates the continue control on `/print/`
- **THEN** they navigate to `/connect/`

#### Scenario: No service wizard
- **WHEN** a visitor views `/print/`
- **THEN** there is no in-page PCBWay-style multi-step order wizard for PartsSetA/B aggregates
