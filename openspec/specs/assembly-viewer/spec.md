# assembly-viewer Specification

## Purpose

Provides a browser 3D assembly view of the stock Tiny Engineer at the SG90 servo preset, driven by a CI-built model from the CAD, with step-by-step build guidance and manual joint posing.

## Requirements

### Requirement: Stock SG90 model only
The assembly viewer MUST show only the stock robot at the SG90 servo preset. It MUST NOT offer other servo presets or optional mods in the viewer UI or as alternate models on the published site.

#### Scenario: Open assembly page
- **WHEN** a user opens the assembly viewer with no hash parameters
- **THEN** the page loads the stock SG90 assembly model

#### Scenario: Legacy preset or mod hash ignored
- **WHEN** a user opens the assembly viewer with a former preset or mod hash (such as `#p=fs0307` or `#m=halloween`)
- **THEN** the viewer still shows only the stock SG90 model and MUST NOT switch to another robot

### Requirement: Build steps
The assembly viewer MUST present the documented build as ordered steps that add or move parts, including focus framing for the active step.

#### Scenario: Step through the build
- **WHEN** a user advances through build steps
- **THEN** the 3D view shows the cumulative robot for that step and highlights the parts that step adds or moves

### Requirement: Stable build step navigation
While build mode is active, the Back, Play, and Next controls MUST stay at the upper part of the build card so their vertical position does not jump when the active step's text or media height changes.

#### Scenario: Long then short step text
- **WHEN** the user moves from a step with long body text to a step with short body text
- **THEN** the Back, Play, and Next controls remain at the same vertical place in the build card

### Requirement: Default colours only
The assembly viewer MUST paint parts with the design default colours. It MUST NOT provide a colour picker, colour share links, colour persistence, or fastener finish selection UI.

#### Scenario: Part colours on load
- **WHEN** the model finishes loading
- **THEN** printed parts and hardware use the fixed default colours for the design

### Requirement: Manual joint posing
The assembly viewer MUST let the user set each servo joint pose on the firmware −1..1 scale at a fixed ±25° angular range. It MUST NOT offer a range picker, playable animation loops, or on-page copy explaining the firmware pose scale or stock angle limits.

#### Scenario: Move one joint
- **WHEN** a user changes a joint slider
- **THEN** the corresponding parts rotate about that servo shaft to the chosen pose at ±25°

#### Scenario: No animation playback
- **WHEN** a user uses the joint controls
- **THEN** there is no control that starts a looping typing, talking, ring, or wave animation

#### Scenario: No range control
- **WHEN** a user views the joint controls
- **THEN** there is no control to choose ±10°, ±25°, or ±40°

### Requirement: Electronics and screws always visible
When a part is shown by the current build step (or the finished robot when build mode is off), electronics and fasteners MUST remain visible. The viewer MUST NOT provide Electronics or Screws visibility toggles.

#### Scenario: Finished robot chrome
- **WHEN** the finished robot is shown
- **THEN** electronics and screws and nuts are visible and there is no Electronics or Screws checkbox

### Requirement: No model inventory status line
After a successful model load, the viewer MUST NOT show a status line that counts printed parts, electronics, or screws and nuts (or the SG90 label in that inventory form). Short loading and error status messages remain allowed.

#### Scenario: Model ready
- **WHEN** the model finishes loading successfully
- **THEN** the status area does not display an inventory summary such as printed / electronics / screws counts

### Requirement: No explode view
The assembly viewer MUST NOT provide an explode control that spreads parts apart from the assembled pose outside of build-step motion.

#### Scenario: Assembled layout
- **WHEN** the user is not advancing a build-step slide-in
- **THEN** parts remain in their assembled (or step) positions without an explode offset control

### Requirement: No print ZIP download
The assembly viewer MUST NOT download a ZIP of print files from the site. Print files remain available from the repository and documentation paths outside the viewer.

#### Scenario: No download control
- **WHEN** a user views the assembly page chrome
- **THEN** there is no control that packs and downloads home-printer or print-service ZIP archives

### Requirement: SG90-only model pipeline
CI MUST build and publish only the stock SG90 assembly GLB (plus its assembly checks). The Pages deploy MUST publish that same stock SG90 model for the viewer and MUST NOT build other servo presets or viewer mods, and MUST NOT stage a `web/print/` tree for viewer download. When assembly model inputs are unchanged, Pages MAY reuse a previously built SG90 GLB. When those inputs change or no matching build is available, Pages MUST rebuild the SG90 GLB (with its checks) and MUST fail the deploy if that rebuild fails.

#### Scenario: Assembly CI build
- **WHEN** the assembly CI job runs successfully
- **THEN** it produces the stock SG90 model artifact and runs the SG90 step and clipping checks (and any remaining static pose checks)

#### Scenario: Pages deploy
- **WHEN** GitHub Pages deploys the site
- **THEN** the assembly viewer can load `models/sg90.glb` and the deploy MUST NOT require staging print files for ZIP download

#### Scenario: Pages deploy with unchanged model inputs
- **WHEN** GitHub Pages deploys the site and the assembly model inputs match a previously successful SG90 build
- **THEN** the assembly viewer can load `models/sg90.glb` without requiring a fresh CAD extract on that deploy, and the deploy MUST NOT require staging print files for ZIP download

#### Scenario: Pages deploy with changed model inputs
- **WHEN** GitHub Pages deploys the site and the assembly model inputs differ from any available previous SG90 build
- **THEN** the deploy rebuilds the stock SG90 GLB with its assembly checks before publish, fails if that rebuild fails, and MUST NOT stage print files for ZIP download
