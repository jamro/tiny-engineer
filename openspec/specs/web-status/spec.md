# web-status Specification

## Purpose

Keeps the web control-panel status bar useful by clearing success messages after a short delay while leaving errors visible until the user dismisses them.

## Requirements

### Requirement: Success status auto-dismisses
When the status bar shows a success (`ok`) message, the UI MUST clear that message automatically after **3 seconds** unless a newer status replaces it first. The dismiss control MUST still clear the message immediately when clicked.

#### Scenario: Ok clears after delay
- **WHEN** an action completes with a success status message
- **THEN** the status bar shows `#status.ok` and clears that message after 3 seconds without further user action

#### Scenario: Newer status cancels the timer
- **WHEN** a success message is showing and another status (loading, ok, or err) is set before 3 seconds elapse
- **THEN** the previous success is replaced and MUST NOT clear the newer message via the old timer

#### Scenario: Manual dismiss still works
- **WHEN** a success message is showing and the user clicks dismiss
- **THEN** the status bar clears immediately

### Requirement: Error status stays until dismissed
When the status bar shows an error (`err`) message, the UI MUST NOT auto-clear it. It MUST remain until the user dismisses it or a newer status replaces it.

#### Scenario: Err remains
- **WHEN** an action fails with an error status message
- **THEN** the status bar keeps `#status.err` visible beyond 3 seconds until dismiss or a newer status
