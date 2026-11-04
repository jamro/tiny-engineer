# Proposal

## Why

The robot already publishes outbound events (`button.click` and later producers) over WebSocket and serial `Evt`, and `tiny-engineer watch` only prints them. Turning a press into a host program is a one-off pipe that dies with the terminal. The host daemon is already the single long-lived process for this machine; reactions belong there so the user installs one login agent and a dead reaction runner cannot diverge from the process that owns the link.

## What Changes

- Add a user-level rules file. Each rule names an event `type` and an argv to spawn when that event arrives. The daemon loads the rules, matches events, and spawns a child. The event loop does not wait on the child.
- The reaction link follows the user-config transport (`http` → one WebSocket on port 81, `serial` → the daemon's USB port). A one-off `--transport` flag does not move that link. The daemon never listens on both, so one device event does not run the action twice.
- HTTP control commands (`anim`, hooks, and the rest) stay direct. They do not start the daemon and do not pass through it.
- With no rules file, or an empty rules list, the daemon behaves as it does today: it does not open a WebSocket and does not take the USB port for reactions.
- **BREAKING** (only while serial reactions are armed): `daemon start` and `daemon restart` open the USB port. Today those commands leave the port closed until a later serial command. `daemon release` and `daemon stop` still close the port and leave the process up, and the reaction holder must not reopen it. The next serial command may open it, as today. `daemon stop --yield` still keeps the port free for 180 seconds.
- `daemon stop` and `daemon release` do not drop an HTTP reaction socket. `daemon disable` still exits the process, which ends reactions on either transport.
- `daemon status` reports the reaction link, the loaded rules, and the last action outcome.
- New CLI surface to list and edit rules (`rules`). Editing signals the running daemon to reload the file. Reload does not bounce the serial port unless the selected transport changed.

## Capabilities

### New Capabilities

- `host-event-reactions`: User rules that spawn a host program when the robot publishes an outbound event, including the rules file, match behavior, child process contract, reaction-link selection, and the rules CLI.

### Modified Capabilities

- `cli-host-daemon`: Armed serial reactions make `daemon start` / `daemon restart` open the USB port; release and stop suspend that auto-open until a later open; status grows reaction fields. Unarmed, the existing port contract stands.

## Impact

- CLI package `packages/tiny-engineer-cli` (daemon server, serial link, new rules loader and process runner, `rules` command, status text, docs in the CLI help).
- User config directory: new `rules.json` beside `env`. Not a new dotenv key.
- Docs that describe daemon start, stop, release, and yield (`docs/serial.md`, `docs/integration.md`).
- No firmware, HTTP route, serial frame, or hook-event change. `tiny-engineer watch` stays a printer. Outbound event envelope stays `v` 1.
