# Proposal

## Why

`tiny-engineer port --probe` still opens the USB CDC device in the short-lived CLI process, while `cli-transport` requires serial I/O through the host daemon. That bypass races a live daemon (busy port, flaky sync) and leaves the last layering hole from the architecture review (5b / option A).

## What Changes

- Route `port --probe` sync attempts through the host daemon; the CLI process MUST NOT open the tty for probe.
- Keep plain `port` listing/scoring in the CLI (`SerialPort.list` only — no exclusive open).
- Preserve probe semantics that matter to users: same candidate order (env → cache → scored others), print per-candidate result, **do not write** the port cache from probe.
- When the daemon already holds a port, probe that path only (sync); do not yank the session to try other paths (would drop `watch` / reactions).
- When no session is open, walk candidates inside the daemon; first sync success leaves the session open (daemon ownership). Auto-start / disable / yield rules match other serial robot entry points.
- Update `cli-transport` and `cli-host-daemon` specs; adjust `docs/serial.md` and CLI tests.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `cli-transport`: `port --probe` is serial I/O through the host daemon; CLI must not open the device for probe.
- `cli-host-daemon`: daemon exposes probe of ordered candidates (sync), with hold-port and cache rules above.

## Impact

- Code: `packages/tiny-engineer-cli` — `commands/port.js`, daemon IPC (`server` / `link` / client RPC), tests (`port-command`, daemon), `docs/serial.md`.
- Commit scope: `integrations` (and `docs` if docs-only split undesired — same change).
- Not **BREAKING** for flags/CLI surface; observable change: successful probe may leave the daemon holding the port (previously closed after sync).
