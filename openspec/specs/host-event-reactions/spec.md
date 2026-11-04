# host-event-reactions Specification

## Purpose

Lets the host daemon run a user program when the robot publishes an outbound event, so a button press can start a script without a second resident process.

## Requirements

### Requirement: Rules file lives in user config
The rules file MUST be named `rules.json` and MUST live in the same user config directory as the user `env` file, including when `TINY_ENGINEER_CONFIG_DIR` is set. A missing file MUST mean zero rules.

#### Scenario: Isolated config directory
- **WHEN** `TINY_ENGINEER_CONFIG_DIR` points at a directory and the user adds a rule
- **THEN** `rules.json` is created in that directory and not in the default user config directory

#### Scenario: Missing file
- **WHEN** the daemon starts and `rules.json` is absent
- **THEN** it loads zero rules and does not open a reaction link

### Requirement: CLI writes the rules file privately
When the CLI creates or rewrites `rules.json` on POSIX, the file mode MUST be `0600`.

#### Scenario: Add creates a private file
- **WHEN** the user adds a rule on POSIX and `rules.json` did not exist
- **THEN** the file mode is `0600`

### Requirement: Rule document shape
A loadable rules file MUST be a JSON object with number `v` equal to 1 and an array `rules`. Each rule MUST include string `on` and a non-empty array of strings `run`. Optional string `source` and optional positive integer `timeoutMs` MUST be honored. Any other field MUST be ignored.

#### Scenario: Minimal rule loads
- **WHEN** the file is `{"v":1,"rules":[{"on":"button.click","run":["/usr/bin/open","-a","Music"]}]}`
- **THEN** the daemon loads one rule for type `button.click`

#### Scenario: Extra field ignored
- **WHEN** a rule includes an unknown field plus valid `on` and `run`
- **THEN** the rule still loads

### Requirement: Invalid rules file keeps the last good set
If `rules.json` does not match the rule document shape, the daemon MUST keep the last successfully loaded set and MUST record a file error. When no set has loaded yet, it MUST use zero rules and MUST still run. A valid file with an empty `rules` array MUST load as zero rules and MUST clear a previous file error.

#### Scenario: Edit breaks the file
- **WHEN** the daemon has loaded two rules and the file is replaced with invalid JSON
- **THEN** those two rules stay active and status reports a rules file error

#### Scenario: First load is invalid
- **WHEN** the daemon starts with invalid `rules.json` and no previous set
- **THEN** it runs with zero rules and reports a rules file error

#### Scenario: Empty list clears reactions
- **WHEN** a valid file whose `rules` array is empty replaces a loaded rule
- **THEN** zero rules are active and the reaction link is not held

### Requirement: Match is exact type and optional source
The daemon MUST start every active rule whose `on` equals the event `type`, in file order. When a rule sets `source`, the event `source` MUST equal it or that rule MUST NOT start. The daemon MUST NOT start a rule for an event whose envelope `v` is not 1.

#### Scenario: Two rules one type
- **WHEN** two rules name `button.click` and the device publishes that type with `v` 1
- **THEN** both actions start, in file order

#### Scenario: Source filter misses
- **WHEN** a rule names `button.click` with `source` `push-button` and the event `source` is different
- **THEN** that rule does not start

#### Scenario: Wrong envelope version
- **WHEN** a published event has `v` other than 1 and a rule names its `type`
- **THEN** no rule starts

### Requirement: Action is a direct spawn
The daemon MUST start `run` as a program plus arguments and MUST NOT invoke a shell. It MUST NOT insert event fields into the argument list. It MUST write the event JSON as one line on the child's stdin and then close stdin. It MUST set `TINY_ENGINEER_EVENT_TYPE`, `TINY_ENGINEER_EVENT_SOURCE`, and `TINY_ENGINEER_EVENT_MS` from the event. The child environment MUST NOT contain the robot access token. The child's working directory MUST be the user home directory.

#### Scenario: Click runs the argv
- **WHEN** `button.click` matches a rule whose `run` is `/usr/bin/open`, `-a`, `Music`
- **THEN** the daemon starts that program with those arguments and does not start a shell

#### Scenario: Event arrives on stdin
- **WHEN** a matching event is published
- **THEN** the child stdin is one JSON line of that event and `TINY_ENGINEER_EVENT_TYPE` is that event's `type`

#### Scenario: Token stays out of the child
- **WHEN** an access token is configured and a rule starts
- **THEN** the child's environment does not contain that token

### Requirement: A reaction child does not block robot commands
The daemon MUST complete a serial robot command while a reaction child is still running. It MUST NOT wait for that child to exit before answering the command.

#### Scenario: Anim during a long action
- **WHEN** a reaction child is still running and the user runs a serial `anim`
- **THEN** `anim` completes without waiting for that child to exit

### Requirement: One child per rule and a global cap
A rule that already has a live child MUST NOT start another. The daemon MUST NOT run more than 8 reaction children at once. A start refused by either limit MUST be skipped and the last-action report MUST show a skip.

#### Scenario: Second click while busy
- **WHEN** `button.click` fires and that rule's child is still running
- **THEN** no second child starts and the last action reports a skip

#### Scenario: Cap blocks a later rule
- **WHEN** eight reaction children are live and another matching rule would start
- **THEN** that start is skipped

### Requirement: Action timeout kills the child
The daemon MUST kill a reaction child that is still running after its limit. The limit MUST be the rule's `timeoutMs` when that value is a positive integer, and MUST be 30000 otherwise. The kill MUST include processes that child spawned.

#### Scenario: Default timeout
- **WHEN** a rule omits `timeoutMs` and its child runs longer than 30 seconds
- **THEN** the daemon kills that child and the last action reports a timeout

#### Scenario: Custom timeout
- **WHEN** a rule sets `timeoutMs` to 1000 and its child runs longer than 1 second
- **THEN** the daemon kills that child

### Requirement: Termination kills reaction children
When the daemon exits because it received SIGTERM, it MUST kill reaction children it spawned.

#### Scenario: Terminate
- **WHEN** the daemon receives SIGTERM while a reaction child is running
- **THEN** that child is killed

### Requirement: Reaction transport follows user config
The reaction link MUST use the user-config transport, not a command-line `--transport` flag. Transport `http` MUST use one WebSocket to port 81 path `/events` and MUST NOT open the serial port for reactions. Transport `serial` MUST use the daemon serial port and MUST NOT open that WebSocket. The daemon MUST NOT hold both reaction links at once. An event observed only on the non-reaction link MUST NOT start a rule.

#### Scenario: Flag does not move reactions
- **WHEN** user-config transport is `http` and the user runs a command with `--transport serial`
- **THEN** reactions stay on the WebSocket and that command may still open the serial port for itself

#### Scenario: Serial event does not double-fire
- **WHEN** user-config transport is `http`, the reaction WebSocket is up, and a serial command has the port open while the device emits the same event on both paths
- **THEN** each rule starts once

### Requirement: HTTP reaction socket
On transport `http`, the daemon MUST open the reaction WebSocket only for an `http:` robot base URL, and MUST append `token` when a Bearer token would be sent on HTTP. It MUST NOT log the token. An `https:` base URL MUST leave the reaction link in error and MUST NOT open a socket. After a disconnect the daemon MUST try again and MUST re-read the token from user config before each attempt.

#### Scenario: HTTPS base
- **WHEN** user-config transport is `http` and the robot base URL uses `https:`
- **THEN** the reaction link is in error and no WebSocket is opened

#### Scenario: Token reconnect
- **WHEN** the access token in user config changes and the reaction socket then drops
- **THEN** the next connection attempt uses the new token and the daemon log does not contain the token value

### Requirement: Unarmed reactions take no link
When zero rules are active, the daemon MUST NOT open a reaction WebSocket and MUST NOT open or hold the serial port for reactions.

#### Scenario: Install with no rules
- **WHEN** the login daemon is running and `rules.json` is missing
- **THEN** the serial port stays closed until a serial command opens it

### Requirement: Armed serial reactions hold the port
When at least one rule is active, user-config transport is `serial`, and reaction open is not suspended, the daemon MUST open the serial port and deliver serial event frames to the rules.

#### Scenario: Start with a serial rule
- **WHEN** a rule is active, user-config transport is `serial`, and the user runs `daemon start`
- **THEN** the daemon opens the serial port without a further robot command

### Requirement: Release and stop suspend reaction open
`daemon release` and `daemon stop` without `--yield` MUST suspend reaction open. The process MUST stay up, the port MUST close, and the daemon MUST NOT open the port for reactions until `daemon start`, `daemon restart`, or a serial command opens it. After that command opens the port, later event frames MUST reach the rules again.

#### Scenario: Release suspends reopen
- **WHEN** armed serial reactions hold the port and the user runs `daemon release`
- **THEN** the port closes, the process stays up, and the daemon does not open the port again until `daemon start`, `daemon restart`, or a serial command

#### Scenario: Serial command resumes delivery
- **WHEN** reaction open is suspended after `daemon release` and the user runs a serial `anim`
- **THEN** that command opens the port and a later event frame starts a matching rule

### Requirement: Yield blocks reaction open
While a yield is active, the daemon MUST NOT open the serial port for reactions.

#### Scenario: Yield blocks the holder
- **WHEN** a yield is active, user-config transport is `serial`, and a rule is active
- **THEN** the daemon does not open the port for reactions

### Requirement: HTTP reactions ignore port release
`daemon release`, `daemon stop` without `--yield`, and `daemon stop --yield` MUST NOT disconnect an HTTP reaction WebSocket. `daemon disable` MUST end reactions by exiting the process and MUST leave them down until `daemon start`.

#### Scenario: Release leaves the socket
- **WHEN** user-config transport is `http`, the reaction socket is connected, and the user runs `daemon release`
- **THEN** the process stays up and the reaction socket stays connected

#### Scenario: Disable ends reactions
- **WHEN** reactions are active and the user runs `daemon disable`
- **THEN** the daemon process exits and a later event does not start a rule until `daemon start`

### Requirement: Reload keeps the same transport link
Reloading rules MUST re-read `rules.json`. When the user-config transport is unchanged, a reload MUST NOT close and reopen the serial port. When the user-config transport changes, the daemon MUST drop the previous reaction link before opening the new one.

#### Scenario: Rule added on an open port
- **WHEN** serial reactions hold the port and the rules file gains another rule and is reloaded
- **THEN** the new rule is active and the serial port stays open across the reload

#### Scenario: Transport switch
- **WHEN** reactions are on HTTP and the user-config transport changes to `serial` and the daemon reloads
- **THEN** the reaction WebSocket is closed before the serial port is opened for reactions

### Requirement: Rules add appends an absolute program
`tiny-engineer rules add <type> -- <program> [args...]` MUST append a rule and MUST store `program` as an absolute path resolved in the user's environment at add time. An absolute `program` MUST be stored as given. A `program` that does not resolve to an absolute path MUST fail and MUST NOT change the file. Add MUST NOT register the login daemon. When a daemon is running, add MUST make that daemon load the new file without a restart.

#### Scenario: Add does not install
- **WHEN** the user runs `rules add` and no login registration exists
- **THEN** the rules file contains the new rule and no login registration is created

#### Scenario: Add reloads a live daemon
- **WHEN** a daemon is running and the user adds a rule for `button.click`
- **THEN** a later `button.click` starts that rule without a daemon restart

#### Scenario: Relative program stored absolute
- **WHEN** the user adds a rule whose program name resolves to an absolute path
- **THEN** the stored `run` array begins with that absolute path

### Requirement: Rules list and remove
`rules list` MUST print each rule's type and program path. `rules remove <index>` MUST delete the rule at the 1-based index printed by `rules list` and MUST reload a running daemon the same way add does. An index that is not in the list MUST fail and MUST NOT change the file.

#### Scenario: Remove keeps the file on a bad index
- **WHEN** the user runs `rules remove` with an index that `rules list` does not show
- **THEN** the command fails and `rules.json` is unchanged

#### Scenario: Remove reloads a live daemon
- **WHEN** a daemon is running with two rules and the user removes index 1
- **THEN** a later event starts only the rule that remains, without a daemon restart

### Requirement: Action failure is logged without secrets
When a reaction child exits non-zero, is skipped, or times out, the daemon log MUST include the event type, the program path, and that outcome. The log MUST NOT include the access token. The daemon MUST consume the child's stderr so a full pipe cannot stall the child.

#### Scenario: Failed program
- **WHEN** a reaction child exits with code 1 and an access token is configured
- **THEN** the daemon log names the event type and the program and does not contain the token value
