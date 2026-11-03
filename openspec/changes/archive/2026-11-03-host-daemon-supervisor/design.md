# Design

## Context

See proposal.md for why. Today `spawnDaemonProcess` in `packages/tiny-engineer-cli/src/daemon/client.js` starts `daemon --serve` detached and unref'd. `stopDaemon` writes `daemon.yield` when asked, then sends `daemon.stop`, and `serveDaemon` closes the serial link and exits. `ensureDaemon` checks the yield file before it connects. `followDaemonChannel` treats `bye` `stop` as the end of `watch` and `bye` `replace` as reconnect. Pre-upload in `scripts/park_servos_before_upload.py` already runs `daemon stop --yield`. IPC files live in `userConfigDir()` (`daemon.sock` or a Windows named pipe, `daemon.json`, `daemon.yield`, `daemon.lock`, `daemon.log`).

## Goals / Non-Goals

**Goals:**

- One owner starts the process when the user has installed the login supervisor. That owner is the wrapper, not the OS unit.
- Serial open is a lease inside that process. Yield and release drop the lease and leave the process up. Yield expiry does not open the port.
- The same wrapper works on macOS, Linux, and Windows. OS units only start that wrapper at login.

**Non-Goals:**

- A system-boot service, a Windows Service, or an install that needs administrator rights.
- OS-level restart (`KeepAlive`, `Restart=`) of the wrapper or the daemon.
- Opening the tty from `daemon start`, `daemon restart`, `daemon install`, or yield expiry.
- Proxying HTTP, a menu-bar app, or more than one robot.
- Changing firmware, the serial frame format, or the park argv.

## Decisions

### D1: Port lease, one sticky opt-out

`daemon release` and `daemon stop` without `--yield` send the same IPC method. The server closes the serial link, sends `link-down` to subscribers, and does not send `bye`. No yield file, no disable file. If `playActive`, the reply is `play-active` and the link stays open. The next serial command may open the port.

`daemon stop --yield` keeps writing `daemon.yield` for 180 seconds in the client, then asks the server to close the link the same way. The process does not exit. When the expiry passes, the server leaves the port closed. A connected `watch` or `monitor` stays up and receives nothing until a later command opens the port. `ensureDaemon` still rejects commands while the file is unexpired, before it uses the socket.

`daemon disable` writes `daemon.disabled` (mode `0600`, no expiry), tells an installed wrapper to stop restarting, sends `bye` `stop`, and exits. `ensureDaemon` treats the file like yield: distinct error, no spawn, unless `daemon start` or `daemon restart` cleared it. `daemon start` clears the file and the yield and does not open the tty. `daemon stop` without yield does not write the file.

**Alternatives:** Sticky `daemon stop` (rejected — that command already means "free the port" in the docs). Reopen the port when yield expires if a subscriber remains (rejected — a long flash or another serial program still holds the tty).

### D2: Wrapper is the only restarter

`daemon install` writes three things into the user config directory: a stable `daemon-wrapper` script, `daemon.target` (absolute `execPath` and CLI script of the installer), and the OS registration. The unit's program path is that wrapper, never a `node_modules` or `npx` path. A second install rewrites `daemon.target` and the node path and leaves one daemon.

The wrapper is the parent. It spawns `daemon --serve` from `daemon.target`. Unexpected child exit after the child has accepted connections: start the child again, with a short backoff. Child exit before it has accepted connections: at most a few tries in a minute, then the wrapper stops spawning. `daemon disable` makes the wrapper exit 0 and leave the disable file in place, so the next login start exits 0 immediately until `daemon start` clears the file and kickstarts the wrapper.

OS registration starts the wrapper at user login and does not restart it:

- macOS: `~/Library/LaunchAgents`, `RunAtLoad`, no `KeepAlive`.
- Linux: `systemd --user`, `WantedBy=default.target`, no `Restart=`.
- Windows: a logon scheduled task, no task restart on failure.

`daemon uninstall` removes the unit and stops the wrapper, which exits the child with `bye` `stop`. It does not write `daemon.disabled`. The next serial command may spawn detached, as today.

When no unit is installed, `spawnDaemonProcess` stays detached and unref'd. A crash then stays down until the next command. New daemon features may assume a live process only after install.

**Alternatives:** OS `KeepAlive` or `Restart=on-failure` plus the wrapper loop (rejected — a non-zero wrapper exit resets the "exit before hello" bound). OS KeepAlive on the daemon binary itself (rejected — replace and disable race the unit).

### D3: Replace updates the target, then the wrapper starts the child

A replace still follows the npm and dev rules. It writes the new script into `daemon.target`, sends `bye` `replace`, and exits the child. It does not spawn and it does not exit the wrapper. The wrapper reads the new target and starts that process. `watch` already reconnects on `bye` `replace`.

If the wrapper is installed and the daemon crashes after hello, `watch` keeps reconnecting until the wrapper's child accepts connections or the wrapper gives up. Without a wrapper, `watch` still retries once.

If no wrapper is running, the CLI spawns the replacement itself, as today.

`daemon install` while a CLI-spawned daemon is up, and no play is active, does the same handoff: one process remains, and it is the wrapper's child. During play, install fails with the play still going.

### D4: Restart

`daemon restart` clears `daemon.disabled` and `daemon.yield`, then cycles the process through the owner from D2 or D3. `--port` is stored as the preferred port for the next open. Restart does not open the tty. It may cut an active play; that matches today's stop-then-start.

## Risks / Trade-offs

- [Users type `daemon stop` and expect the process to die] → The process stays. Help and docs say stop frees the port. `daemon disable` is the opt-out. `watch` stays up across stop.
- [A dead wrapper stays down until next login] → `daemon start` kickstarts it. The OS unit does not restart it, so a failed boot cannot spin.
- [Wrapper and a detached daemon both start] → Install hands off before the wrapper binds the socket. While a unit is installed, `ensureDaemon` must not call `spawnDaemonProcess`.
- [Login PATH is only `/usr/bin:/bin`] → The unit stores absolute `node` and wrapper paths. `daemon.target` stores absolute paths too. A second install refreshes them.
- [Rollback to a CLI that does not know `daemon.disabled`] → That CLI ignores the file and can auto-start. Uninstall removes the unit.

## Migration Plan

Ship the CLI, the wrapper, and the doc change together. Existing daemons keep today's exit-on-stop until the user upgrades and the next replace or `daemon start` runs the new process. After that, `daemon stop` without `--yield` leaves the process up. `daemon stop --yield` argv in the pre-upload script does not change. Rollback is uninstall plus the previous CLI. No firmware flash and no config migration beyond ignoring unknown files.

## Open Questions

None.
