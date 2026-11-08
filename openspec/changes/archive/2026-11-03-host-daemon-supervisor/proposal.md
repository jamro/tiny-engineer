# Proposal

## Why

The host daemon dies on reboot, on crash, and on every stop, including the upload yield. The next serial command pays a process start, and nothing can assume the process is there. A login supervisor would bring it back, but today's stop, yield, and replace all exit the process while the CLI also spawns one, so the two race for the socket.

## What Changes

- Split process lifetime from the serial-port lease. After `daemon install`, new daemon work may assume the process is up. It must not assume the tty is open. Without an install, a crash still leaves the process down until the next serial command.
- `daemon stop` without `--yield` is the same as `daemon release`. The process stays up, the port closes, and the next serial command may open it immediately. Neither command reopens the port by itself.
- `daemon disable` is the sticky opt-out. The process exits, an installed supervisor stays down, and serial commands do not auto-start until `daemon start`. `watch` ends on disable and on uninstall of a supervisor-owned daemon. Uninstall does not record a disable.
- `daemon stop --yield` keeps the process, closes the port, and refuses serial open for 180 seconds. The pre-upload argv stays `daemon stop --yield`. When the window ends, the port stays closed until a later command asks for it, including while `watch` or `monitor` is still connected.
- `daemon start` clears a disable and a yield. It does not open the tty. `daemon restart` clears both, then starts a fresh process. `--port` is the port that process prefers on the next open.
- Optional login install (macOS LaunchAgent, systemd user unit, Windows logon task). The unit only starts a stable wrapper at login. It does not restart that wrapper. The wrapper is the only crash restarter, and its program path is not a `node_modules` or `npx` path. Replace writes the new package target and exits the daemon. The wrapper stays and starts that target. A second install refreshes the target and the node path and does not start a second daemon. Without an install, the CLI keeps spawning a detached process as it does today. A disable still blocks that spawn.
- Replace still does not cut an active play. npm versus dev replace rules stay.

## Capabilities

### New Capabilities

### Modified Capabilities

- `cli-host-daemon`: Process lifetime versus serial lease, `daemon release` and `daemon stop` as the same port release, `daemon disable` as the sticky opt-out, yield that keeps the process and does not reopen the port, login wrapper as sole spawner and sole restarter.
- `outbound-events`: Serial `watch` ends on `daemon disable` and on uninstall of a supervisor-owned daemon. Release, plain stop, and yield leave `watch` running. A supervisor restart keeps `watch`. A crash without a supervisor still allows one reconnect.
- `servos-park`: Pre-upload yield frees the serial port and refuses serial open while the host daemon process stays up. Expiry does not open the port again.
- `cli-transport`: Serial hooks and doctor auto-start through the host daemon except while a disable or a yield is active.

## Impact

- Package: `packages/tiny-engineer-cli` daemon commands, spawn path, IPC, and tests. No new npm dependency for IPC.
- Script: `scripts/park_servos_before_upload.py` keeps `daemon stop --yield`. The process no longer exits on that call, and expiry does not reopen the port.
- Docs: `docs/serial.md`, `docs/integration.md`, CLI help. Everyday "free the port" is `daemon release` or `daemon stop`. `daemon disable` is the opt-out.
- Firmware, HTTP routes, and the serial frame format stay as they are.
