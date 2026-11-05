# Spec Delta

## MODIFIED Requirements

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
