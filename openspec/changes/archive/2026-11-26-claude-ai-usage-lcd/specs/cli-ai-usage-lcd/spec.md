# Spec Delta

## Purpose

Show remaining Claude Code plan budget on the LCD1602 after a finished turn, using the same opt-in and the same two lines as the Cursor readout.

## ADDED Requirements

### Requirement: Claude budget update is opt-in
The CLI MUST update the LCD from Claude usage only when `TINY_ENGINEER_AI_USAGE_LCD` is exactly `1`. Any other value, including unset, MUST skip the Claude usage lookup and MUST NOT post LCD text for this feature.

#### Scenario: Flag unset
- **WHEN** a Claude Code turn completes and `TINY_ENGINEER_AI_USAGE_LCD` is unset
- **THEN** the CLI does not post LCD text for budget

#### Scenario: Flag off
- **WHEN** a Claude Code turn completes and `TINY_ENGINEER_AI_USAGE_LCD` is `0`
- **THEN** the CLI does not post LCD text for budget

### Requirement: Completed Claude turn writes one percent
When the flag is `1` and the Claude Code hook maps the turn to the `ring` animation, the CLI MUST post LCD line 1 `CLAUDE BUDGET` and line 2 the remaining percent with two decimal places and a trailing `%`, right-aligned. A Cursor turn MUST post line 1 `CURSOR BUDGET` with the same line 2 format. Events that do not map to `ring` MUST NOT post that budget text.

#### Scenario: Stop posts the lines
- **WHEN** the flag is `1` and a Claude Code `Stop` turn maps to `ring` with a known remaining percent
- **THEN** the LCD post sets line 1 to `CLAUDE BUDGET` and line 2 to that percent, right-aligned

#### Scenario: Failed turn skips budget
- **WHEN** the flag is `1` and the Claude Code event maps to an animation other than `ring`
- **THEN** the CLI does not post budget text

### Requirement: Remaining percent uses the fullest window
The remaining percent MUST be `100` minus the highest utilization among every usage window in the Claude subscription payload, then clamped to at least `0`, then rounded to two decimal places. A payload with no numeric window MUST NOT post LCD text.

#### Scenario: Weekly is fuller than the session
- **WHEN** the session window utilization is 20 and the weekly window utilization is 40
- **THEN** the posted line 2 is `60.00%`

#### Scenario: A per-model window is fullest
- **WHEN** session utilization is 10, weekly utilization is 20, and a per-model window utilization is 90
- **THEN** the posted line 2 is `10.00%`

#### Scenario: Over-full window clamps to zero
- **WHEN** the highest utilization is 100 or greater
- **THEN** the posted line 2 is `0.00%`

#### Scenario: Enterprise extra usage is the only window
- **WHEN** plan windows are null, extra usage utilization is 19.77, and spend percent is 20
- **THEN** the posted line 2 is `80.23%`

#### Scenario: No windows
- **WHEN** the usage payload has no numeric utilization
- **THEN** the CLI does not post LCD text

### Requirement: Claude budget lookup fails open
A missing Claude Code login, an API-key login with no subscription windows, an unusable token, a failed or slow usage request, a disabled LCD extension, or a robot error MUST NOT throw, MUST NOT change the hook exit code from success, and MUST NOT post budget text when no percent was obtained. The CLI MUST NOT write or refresh Claude Code credentials.

#### Scenario: No subscription login
- **WHEN** the flag is `1`, the turn maps to `ring`, and no usable subscription login is present
- **THEN** the hook exits successfully and the CLI does not post budget text

#### Scenario: Usage request fails
- **WHEN** the flag is `1`, the turn maps to `ring`, and the usage request fails or exceeds the lookup time cap
- **THEN** the hook exits successfully and the CLI does not post budget text

#### Scenario: Credentials stay untouched
- **WHEN** the CLI reads a Claude Code login to compute the percent
- **THEN** it does not write or refresh that login

### Requirement: Cursor readout stays independent
The existing Cursor remaining-percent lookup and its LCD post MUST keep their current inputs and formula. A finished Cursor turn and a finished Claude Code turn MUST each post their own percent. The later post overwrites the two LCD lines.

#### Scenario: Cursor turn still uses Cursor usage
- **WHEN** the flag is `1` and a Cursor turn maps to `ring`
- **THEN** the posted percent comes from Cursor usage, not from Claude windows

#### Scenario: Later turn overwrites the lines
- **WHEN** a Claude budget post is followed by a Cursor budget post
- **THEN** the LCD lines show the Cursor percent

### Requirement: Setup offers budget monitoring only for IDEs that support it
Interactive setup MUST ask about LCD AI budget only when a selected IDE is Cursor or Claude Code. The question MUST name those selected IDEs. Antigravity, alone, MUST NOT be asked and MUST NOT write `TINY_ENGINEER_AI_USAGE_LCD`. When the flag is enabled, setup MUST print sign-in steps for each selected supporting IDE: Cursor uses the account already on the machine, and Claude Code requires `/login` rather than an API key.

#### Scenario: Antigravity skips the option
- **WHEN** setup is run for Antigravity only, including with `--ai-usage-lcd`
- **THEN** the CLI does not ask about AI budget and does not write `TINY_ENGINEER_AI_USAGE_LCD`

#### Scenario: Cursor enable prints Cursor steps
- **WHEN** setup enables LCD AI budget for Cursor
- **THEN** the CLI prints how Cursor finds the existing session and does not tell the user to run Claude `/login`

#### Scenario: Claude enable prints login steps
- **WHEN** setup enables LCD AI budget for Claude Code
- **THEN** the CLI prints that `/login` is required and that an API key will not update the screen
