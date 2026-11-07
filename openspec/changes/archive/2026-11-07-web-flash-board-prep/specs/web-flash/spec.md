# Spec Delta

## Purpose

Helps builders prepare the main control board and install stock firmware from the public GitHub Pages flash stage over USB in the browser, before mechanical assembly.

## ADDED Requirements

### Requirement: Flash stage presents prepare then install
The `/flash/` page MUST show a board-preparation section above the firmware install controls. The page MUST NOT present Connect & install as the first instructional content after the stage title.

#### Scenario: Prep appears first
- **WHEN** a visitor opens `/flash/`
- **THEN** a prepare-the-board section appears before the release picker and Connect & install control

#### Scenario: Install remains available
- **WHEN** a visitor scrolls or continues past preparation
- **THEN** they can still select a release and run browser USB install without leaving `/flash/`

### Requirement: Two-step guided board preparation
Board preparation MUST be a two-step guided sequence with Previous and Next controls. Step 1 MUST instruct seating the ESP32 module into the main control board sockets and aligning with the board USB mark. Step 2 MUST instruct plugging the USB data cable into the board USB-C and MUST tell the visitor not to use the USB port on the ESP32 module.

#### Scenario: Step 1 seating and orientation
- **WHEN** the visitor is on preparation step 1
- **THEN** the step title and body tell them to seat the module on the main board and match the USB mark on the board

#### Scenario: Step 2 board USB only
- **WHEN** the visitor advances to preparation step 2
- **THEN** the step tells them to plug USB into the board and not into the ESP32 module port

#### Scenario: Step navigation
- **WHEN** the visitor uses Previous or Next on the preparation guide
- **THEN** the active step content and 3D view update to that step without leaving `/flash/`

### Requirement: Visual prep uses assembly model electronics
The preparation 3D view MUST load the same published stock SG90 assembly model used by `/assembly/`. While preparation is shown, the view MUST include the main control board and ESP32 module (with pin headers as modeled) and MUST NOT show the assembled desk or full robot as the primary scene.

#### Scenario: Shared model asset
- **WHEN** the preparation view loads successfully
- **THEN** it uses the published assembly SG90 model from the site (same origin as `/assembly/`)

#### Scenario: Bench electronics only
- **WHEN** preparation is visible
- **THEN** the framed scene is the board and ESP32 electronics on the bench, not the finished desk assembly

### Requirement: Seat animation and USB port callouts
Entering or focusing preparation step 1 MUST animate the ESP32 module seating into the board sockets from above. Preparation step 2 MUST visually distinguish the correct board USB-C from the incorrect module USB-C (callouts or equivalent highlights). The guide MUST remain usable if WebGL or the model fails to load, with the two step texts still readable.

#### Scenario: Seat motion on step 1
- **WHEN** preparation step 1 becomes active
- **THEN** the module moves from an elevated position into the seated pose on the board

#### Scenario: Port distinction on step 2
- **WHEN** preparation step 2 becomes active
- **THEN** the view marks the board USB-C as the correct plug target and the module USB-C as incorrect

#### Scenario: Graceful degradation
- **WHEN** WebGL is unavailable or the model fails to load
- **THEN** the page still shows the two preparation step texts and the install controls remain usable

### Requirement: Beginner prep copy
Preparation headings, step titles, and primary step body copy MUST use plain language aimed at visitors who have never soldered. Copy MUST lead with the job (seat the module, plug into the board) before chip or vendor identifiers. Header soldering instructions MUST NOT be required as a preparation step on `/flash/`.

#### Scenario: Outcome-first wording
- **WHEN** a solder-never visitor reads the preparation steps
- **THEN** each step explains what to do in everyday words before optional technical names

#### Scenario: Headers out of flash prep
- **WHEN** the visitor follows the two preparation steps
- **THEN** they are not required to complete pin-header soldering as a flash-page step

### Requirement: Browser USB install preserved
The flash page MUST keep Chrome/Edge browser USB install for stock releases via the existing web install control, including release selection and the parts/offset image map for the selected release. Prep MUST NOT remove or replace that install path.

#### Scenario: Release install still works
- **WHEN** a visitor selects a flashable release and activates Connect & install in a supported browser
- **THEN** the page still drives browser USB install using that release’s manifest

#### Scenario: Image map still reflects release
- **WHEN** a visitor selects a release
- **THEN** the image-map table still lists that release’s parts and offsets
