# Spec Delta

## ADDED Requirements

### Requirement: Servo centering callout before shaft attach
When a build step requires electrically centering a servo before attaching a printed part to its shaft, the assembly viewer MUST show a distinct centering callout in the build card, separate from ordinary step body text.

#### Scenario: Centering-marked step shows callout
- **WHEN** the user opens a build step marked as requiring servo centering before attach
- **THEN** the build card shows a compact centering callout distinct from the step body text
- **AND** the full centering procedure is collapsed until the user expands the callout

#### Scenario: Ordinary step has no centering callout
- **WHEN** the user opens a build step that does not require centering before attach
- **THEN** the build card does not show the centering callout

#### Scenario: Navigation stays put when callout appears
- **WHEN** the user moves from a step without a centering callout to a step with one
- **THEN** the Back, Play, and Next controls remain at the same vertical place in the build card

### Requirement: Centering callout procedure matches assembly guide
The centering callout MUST include the shared procedure from the mechanical assembly guide §2a: plug the servo into any PCA9685 channel, join the setup access point, open Assembly support, press Move all to 90°, optionally use Identify, then unplug before the next awkward join. It MUST warn not to finish the setup wizard until the guide’s final setup section. That procedure MAY be revealed by expanding the compact callout rather than shown fully by default.

#### Scenario: Callout lists Move all to 90° path
- **WHEN** the user expands a centering callout
- **THEN** it instructs the builder to use Assembly support → Move all to 90° on the setup AP configurator and to unplug after centering
- **AND** it warns not to finish the setup wizard until the final setup section

### Requirement: Centering callout does not pose joints in the viewer
Satisfying a centering-marked step MUST NOT animate or rotate parts about servo shafts in the 3D view; the published model stays at its fixed assembled pose.

#### Scenario: Callout without joint motion
- **WHEN** the user opens a centering-marked build step
- **THEN** the 3D model remains at its fixed assembled pose with no joint-pose control

### Requirement: Centering steps show the bench board and ESP32
When a build step is marked for servo centering, the 3D view MUST show the main control board together with the ESP32 module (and its pin headers), even if those parts have not been added yet in the build sequence. If the desk or lamp parts are already shown, they MUST be drawn as ghosts so the board stays readable. Framing for that step MUST include the board and ESP32.

#### Scenario: Early centering previews the board
- **WHEN** the user opens a centering-marked step before the electronics are seated in the desk
- **THEN** the main control board, ESP32, and pin headers are visible in the 3D view
- **AND** the view framing includes those parts

#### Scenario: Later centering ghosts the desk over the board
- **WHEN** the user opens a centering-marked step after the desk stack and board are part of the build
- **THEN** the main control board and ESP32 remain visible
- **AND** the desk (and lamp cover parts, when present) are drawn as ghosts

### Requirement: Bench window section before first centering attach
The ordered build steps MUST include a Bench window section that appears after closing the head with the hat and before the first step that presses a part onto a centered servo shaft. That section MUST teach the bench-window rules: electronics stay on the desk until the desk electronics step, use Assembly support only for Move all to 90° / Identify, and do not finish the wizard until the final setup section.

#### Scenario: Bench window precedes neck attach
- **WHEN** the user steps through the build in order
- **THEN** a Bench window section appears after the hat section and before Neck is pressed onto a servo shaft

### Requirement: Centering coverage for shaft-attach joints
The shipped build steps MUST mark every step that presses a part onto a servo shaft after centering as requiring the centering callout, matching the mechanical guide: Neck on the neck servo, Neck onto the head servo, BellyJoint on the body servo, and the arms on the hand servos. Seating a servo into a printed pocket without attaching a shaft part MUST NOT require the centering callout.

#### Scenario: Shaft-attach steps are marked
- **WHEN** the shipped step data is validated for centering coverage
- **THEN** the Neck attach, Head–Neck join, BellyJoint attach, and arm mount steps are marked for the centering callout
- **AND** the chest side-pocket seating steps are not marked
