# Spec Delta

## ADDED Requirements

### Requirement: Fixed assembled pose
The assembly viewer MUST show the stock robot at the assembled pose baked into the published model. It MUST NOT provide joint sliders, a 90° pose reset, or any other control that rotates parts about servo shafts at runtime.

#### Scenario: Finished robot stays assembled
- **WHEN** a user views the finished robot with build mode off
- **THEN** every part remains in its assembled model pose and there is no joint pose control in the page chrome

#### Scenario: Build steps do not reorient joints
- **WHEN** a user advances through build steps
- **THEN** parts move only for that step's add/move/slide-in offsets and MUST NOT be rotated by a joint-pose system

## REMOVED Requirements

### Requirement: Manual joint posing
**Reason**: Interactive joint posing is unused; keeping kinematics (viewer pivots, pose modules, and CI pose sweeps) adds maintenance without product value after the SG90-only simplify cut.
**Migration**: Use the physical robot / firmware pose APIs for servo motion. The assembly viewer is build guidance at the fixed assembled pose only.

## MODIFIED Requirements

### Requirement: SG90-only model pipeline
CI MUST build and publish only the stock SG90 assembly GLB (plus its assembly checks). The Pages deploy MUST publish that same stock SG90 model for the viewer and MUST NOT build other servo presets or viewer mods, and MUST NOT stage a `web/print/` tree for viewer download. When assembly model inputs are unchanged, Pages MAY reuse a previously built SG90 GLB. When those inputs change or no matching build is available, Pages MUST rebuild the SG90 GLB (with its checks) and MUST fail the deploy if that rebuild fails. Assembly checks for the published model MUST include step and clipping validation and MUST NOT require a servo joint-pose sweep.

#### Scenario: Assembly CI build
- **WHEN** the assembly CI job runs successfully
- **THEN** it produces the stock SG90 model artifact and runs the SG90 step and clipping checks without a joint-pose sweep

#### Scenario: Pages deploy
- **WHEN** GitHub Pages deploys the site
- **THEN** the assembly viewer can load `models/sg90.glb` and the deploy MUST NOT require staging print files for ZIP download

#### Scenario: Pages deploy with unchanged model inputs
- **WHEN** GitHub Pages deploys the site and the assembly model inputs match a previously successful SG90 build
- **THEN** the assembly viewer can load `models/sg90.glb` without requiring a fresh CAD extract on that deploy, and the deploy MUST NOT require staging print files for ZIP download

#### Scenario: Pages deploy with changed model inputs
- **WHEN** GitHub Pages deploys the site and the assembly model inputs differ from any available previous SG90 build
- **THEN** the deploy rebuilds the stock SG90 GLB with its assembly checks before publish, fails if that rebuild fails, and MUST NOT stage print files for ZIP download
