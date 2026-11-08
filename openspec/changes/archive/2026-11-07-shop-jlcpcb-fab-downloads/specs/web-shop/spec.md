# Spec Delta

## MODIFIED Requirements

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

## ADDED Requirements

### Requirement: Full guide remains available for self-export
The shop page MUST keep a link to the full order guide so builders who prefer (or need) local KiCad export can follow that path. The shop MUST NOT remove documentation access solely because downloads exist.

#### Scenario: Full guide link present
- **WHEN** a visitor views the JLCPCB board lane
- **THEN** a link to the full order-main-control-board documentation remains available
