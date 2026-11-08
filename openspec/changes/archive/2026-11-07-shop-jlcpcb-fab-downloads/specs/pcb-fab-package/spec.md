# Spec Delta

## Purpose

Publishes a JLCPCB-oriented manufacturing package for the main control board on the public site so builders can order PCBA without installing KiCad.

## ADDED Requirements

### Requirement: Build-time package for main-control-board
The Pages (or equivalent site) build MUST generate a manufacturing package for `hardware/boards/main-control-board/` from the KiCad sources at that commit. The package MUST NOT be committed under `hardware/**/production/` as the source of truth for publication.

#### Scenario: Fresh deploy after board change
- **WHEN** site deploy runs for a commit that includes the main-control-board KiCad project
- **THEN** the build produces Gerber archive, `bom.csv`, and `positions.csv` derived from those sources

### Requirement: Published paths under shop fab
The generated package MUST be published on the deployed site under a stable path rooted at `/shop/fab/main-control-board/` (or the same-origin equivalent under the Pages `web/` root). Builders MUST be able to download at least: a Gerber ZIP, `bom.csv`, and `positions.csv`.

#### Scenario: Same-origin downloads
- **WHEN** a visitor requests the published Gerber ZIP, `bom.csv`, and `positions.csv` URLs for main-control-board
- **THEN** each responds successfully from the deployed site without requiring a GitHub Release asset or a local KiCad install

### Requirement: JLCPCB-oriented file contents
`bom.csv` and `positions.csv` MUST use JLCPCB web-upload oriented columns and conventions compatible with the project’s documented Fabrication Toolkit export path (including LCSC part identification in the BOM). The Gerber archive MUST be suitable for JLCPCB PCB upload for this board.

#### Scenario: BOM columns
- **WHEN** a builder opens the published `bom.csv`
- **THEN** it includes designator, footprint, quantity, value, and LCSC part identification columns usable for JLCPCB BOM upload

#### Scenario: Positions columns
- **WHEN** a builder opens the published `positions.csv`
- **THEN** it includes designator, mid X/Y, rotation, and layer fields usable as a JLCPCB CPL / pick-and-place upload

### Requirement: Always latest from main deploy
Published fab files MUST track the board sources of the commit being deployed to the public site. The system MUST NOT require a GitHub Release tag to refresh the package for builders on the default site.

#### Scenario: Board push refreshes package
- **WHEN** main-control-board KiCad sources change on the branch that deploys Pages and deploy completes
- **THEN** the public `/shop/fab/main-control-board/` files reflect that newer board revision

### Requirement: Deploy fails closed on missing package
If package generation fails, the site deploy MUST fail rather than publish a shop page that advertises fab downloads without the files.

#### Scenario: Generator error
- **WHEN** manufacturing package generation exits non-zero during site deploy
- **THEN** the deploy does not complete successfully with missing fab download files
