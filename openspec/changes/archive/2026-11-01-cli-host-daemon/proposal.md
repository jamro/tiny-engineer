# Proposal

## Why

USB CDC allows one open. `watch` and `monitor` hold that port until Ctrl+C, so a second process (`anim`, hooks, `play`) cannot connect. The serial protocol already multiplexes requests, events, and logs inside one process. The missing piece is one long-lived host process that owns exclusive links and serves short-lived CLI invocations.

## What Changes

- Add a host daemon subcommand of the existing `tiny-engineer` binary. The process is a general local session (RPC, subscriptions, lifecycle). Serial port ownership is the only link in this change. HTTP stays in the short-lived CLI and does not start or use the daemon.
- The daemon stays up until `daemon stop`, crash, or an in-place replace. It does not idle-exit. A serial command auto-starts it when it is not running (startup delay is acceptable). `daemon start` when already running, and `daemon stop` when not running, both succeed.
- Short serial commands, `watch`, `monitor`, and serial hooks talk to the daemon over a local socket (named pipe on Windows). They do not open the tty themselves.
- Replace a running daemon when the caller is a newer npm package, or when a dev checkout (`packages/tiny-engineer-cli` outside `node_modules`) should take over. Do not flap between `npx` cache directories of the same version. Do not let an npm client replace a dev-channel daemon.
- **BREAKING** (host serial ownership): after a serial command returns, the daemon keeps the CDC open until `daemon stop`. Other programs (esptool, PlatformIO monitor) see a busy port while it runs.
- Pre-upload park still runs, then asks the daemon to stop and to refuse auto-start for a short yield window so the flash tool can open the port. Park or stop failure still does not abort upload.
- User-facing installs stay on the published npm package (`npx -y tiny-engineer` / global install). A checkout launched as `node packages/tiny-engineer-cli/bin/tiny-engineer.js` is a first-class dev channel on the same socket.

## Capabilities

### New Capabilities

- `cli-host-daemon`: Local host daemon lifecycle, socket RPC, serial link ownership, version and dev/npm replace rules, and the upload yield window.

### Modified Capabilities

- `cli-transport`: Serial robot commands and `monitor` use the host daemon instead of opening the port in the short-lived process. HTTP behavior is unchanged.
- `outbound-events`: Serial `watch` subscribes through the daemon. It exits on an intentional stop and resubscribes across a replace.
- `servos-park`: Pre-upload stops the daemon and sets the yield window after the park attempt.

## Impact

- Package: `packages/tiny-engineer-cli` (new daemon module, serial connect path, `watch` / `monitor`, doctor/status text, tests). No new npm dependency required for the socket (`node:net`).
- Script: `scripts/park_servos_before_upload.py` stops the daemon with a yield after park.
- Docs: `docs/serial.md`, `docs/integration.md`, CLI help. Hook timeout stays 30s.
- Firmware and the USB serial frame format stay as they are. No HTTP API change.
