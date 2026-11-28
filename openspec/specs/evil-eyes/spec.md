# evil-eyes Specification

## Purpose

Adds an OLED face style that always draws sharp, menacing scowling silhouettes while the robot keeps using its existing animations and motion.

## Requirements

### Requirement: Evil is a valid eyes_style value
The device MUST accept `eyes_style` value `evil` on settings read and write paths that already accept other eye styles. An unknown style MUST still be rejected. The default when unset MUST remain `classic`.

#### Scenario: Save evil style
- **WHEN** a client posts `{"eyes_style":"evil"}`
- **THEN** the value is stored and `GET /settings` reports `eyes_style` as `evil`

#### Scenario: Reject unknown style
- **WHEN** a client posts an `eyes_style` that is not a registered style id
- **THEN** the request is rejected as invalid and no settings are saved

#### Scenario: Default unchanged
- **WHEN** settings load on a device that never saved `eyes_style`
- **THEN** the effective style is `classic`

### Requirement: Evil style applies immediately
Changing `eyes_style` to or from `evil` MUST take effect on the OLED without reboot, same as other eye styles.

#### Scenario: Switch to evil without reboot
- **WHEN** the device is running another style and a client sets `eyes_style` to `evil`
- **THEN** the next eye redraw uses the evil look

### Requirement: Evil open eyes are solid white silhouettes
When `eyes_style` is `evil` and an eye is open, each eye MUST be one solid white filled shape on black with no pupil, iris, outline, gradient, or separate brow stroke.

#### Scenario: No separate pupil or outline
- **WHEN** `eyes_style` is `evil` and an eye is drawn open
- **THEN** that eye is one filled white shape (not a rim around a black pupil)

### Requirement: Evil open eyes use the scowling silhouette geometry
When `eyes_style` is `evil` and an eye is open, the outer corner MUST end in a sharp tip higher than the rest of that eye, and the inner corner MUST sit lower and stay rounded. The top edge MUST show a concave brow notch; the bottom edge MUST be a smooth convex curve. Left and right MUST be horizontal mirrors. At the default open pose, the gap between the inner corners MUST be roughly half an eye width.

#### Scenario: Open idle silhouette
- **WHEN** `eyes_style` is `evil` and the eyes are in an open idle pose
- **THEN** the OLED shows two mirrored solid white scowling silhouettes on black, with pointed high outer tips, lower rounded inners, concave tops, and convex bottoms

### Requirement: Evil look stays across animations
When `eyes_style` is `evil`, every animation mode MUST keep that silhouette language (no cute or smiling face, and no switch to another style’s face set). Pose may move, scale, or blink the same shapes.

#### Scenario: Every animation stays evil
- **WHEN** `eyes_style` is `evil` and any existing animation runs
- **THEN** the OLED face stays in the evil silhouette language

### Requirement: Evil eyes track classic pose and blink
Evil eyes MUST follow the same pose and blink pipeline as classic for position, size/height, glances, and lid open amount. Height that renders as closed MUST draw no white eye (full blink or sleep shut).

#### Scenario: Blink closes the silhouette
- **WHEN** `eyes_style` is `evil` and a blink reaches fully closed
- **THEN** neither eye shows white fill for that frame

#### Scenario: Glance moves both silhouettes
- **WHEN** `eyes_style` is `evil` and the classic pose glances horizontally
- **THEN** both evil eyes move with that pose

### Requirement: Evil eyes handle sleep and dead like other pose styles
When `eyes_style` is `evil`, sleep close/open MUST collapse or restore silhouette height with the sleep lid phase. During the dead X hold, the OLED MUST show X marks instead of open silhouettes (same idea as dots/classic).

#### Scenario: Sleep blanks the display path
- **WHEN** `eyes_style` is `evil` and sleep lids reach shut
- **THEN** the evil style draws no open silhouettes for that closed phase

#### Scenario: Dead X hold
- **WHEN** `eyes_style` is `evil` and the dead animation is in its X hold
- **THEN** the OLED shows X marks rather than open evil eyes

### Requirement: Config and docs list evil
The on-device Config eyes-style control MUST offer `evil` beside the existing styles. Public API documentation that lists `eyes_style` values MUST include `evil` and a short description of the look.

#### Scenario: Config select includes evil
- **WHEN** the user opens Config eyes style
- **THEN** `evil` is available as a choice

#### Scenario: API docs mention evil
- **WHEN** a reader checks the documented `eyes_style` values
- **THEN** `evil` is listed with the other styles
