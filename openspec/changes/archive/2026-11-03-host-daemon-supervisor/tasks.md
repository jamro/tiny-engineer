# Tasks

## 1. Port lease and disable

- [x] 1.1 Make `daemon release` and `daemon stop` without `--yield` the same port release. Neither sends `bye`, writes a yield, or writes a disable. Both return `play-active` without closing the port when a play is active. `daemon stop --yield` still writes a 180s `daemon.yield`, leaves the process running, and does not open the port when the window ends, even if `watch` or `monitor` is connected. Verify in `packages/tiny-engineer-cli/test/daemon.test.js` with a temp config dir: stop without yield then a serial request reuses the same pid and opens the port; release during play fails and the fake port stays open; yield then status still shows the daemon; a serial command during yield fails with the yield error and does not spawn; after expiry the port stays down while a subscriber is connected.
- [x] 1.2 Add `daemon disable`. It writes `daemon.disabled` (mode `0600`, no expiry) even when no daemon is running, sends `bye` `stop`, and exits. `ensureDaemon` fails with a distinct disable error and does not spawn while the file is present, unless `daemon start` or `daemon restart` cleared it. `daemon start` clears the file and the yield and does not open the tty. `daemon restart` clears both and leaves one daemon running; `--port` is only the preferred port for the next open. Verify: disable then `anim` fails and does not spawn; `daemon stop` while absent does not write the file; `daemon start` then status shows a running daemon with the port down; restart after disable shows a running daemon.
- [x] 1.3 Update `tiny-engineer daemon` help so it lists `release`, `disable`, `install`, `uninstall`, and says plain `stop` matches `release` and stays up until the process is disabled. Verify `tiny-engineer daemon --help` mentions `release`, `disable`, and that plain `stop` does not stay down.

## 2. Watch and monitor

- [x] 2.1 Keep serial `watch` and `monitor` running across `daemon release`, `daemon stop` without `--yield`, and `daemon stop --yield`. After yield expiry they stay up and print nothing until a later command opens the port. `bye` `stop` from `daemon disable` or from uninstall of a supervisor-owned daemon ends `watch` without spawning. Uninstall does not write `daemon.disabled`, and a later serial command may auto-start. `bye` `replace` still resubscribes. With no supervisor, a crash still retries once. With a supervisor that brings the daemon back, `watch` stays up and prints later frames. Verify in `packages/tiny-engineer-cli/test/watch.test.js` (and the monitor test if it covers `bye`).

## 3. Login supervisor

- [x] 3.1 Add a config-dir wrapper that reads `daemon.target` (absolute `execPath` and CLI script) and spawns `daemon --serve`. Unexpected exit after the child has accepted connections starts it again with backoff. Exit before accept is bounded (a few tries in a minute, then the wrapper stops spawning). A `daemon.disabled` file makes the wrapper exit 0 without spawning. Verify with a temp dir and a fake child: crash after hello comes back; three fast exits before hello stop; a disabled file does not spawn.
- [x] 3.2 Add `daemon install` and `daemon uninstall` with a per-OS registration behind a small interface. macOS LaunchAgent is `RunAtLoad` with no `KeepAlive`. systemd user unit has no `Restart=`. Windows is a logon task with no failure restart. Install MUST NOT register a system-boot service. Paths in the unit are absolute. A second install refreshes `daemon.target` and the node path and leaves one daemon. Install while a play is active fails and leaves that daemon. Install while an orphan daemon is up hands off so one supervisor-owned process remains. Uninstall removes the unit, ends the wrapper's child with `bye` `stop`, and does not write `daemon.disabled`. Verify the unit text or argv with a fake registrar on a temp dir, plus the second-install target refresh, the play-active failure, and the handoff leaving a single hello.
- [x] 3.3 When a supervisor is installed, `ensureDaemon` and replace MUST NOT call `spawnDaemonProcess`. Replace writes the new package into `daemon.target`, sends `bye` `replace`, exits the child, and the same wrapper starts that target. With no supervisor, the CLI still spawns detached. Verify: an installed fake supervisor receives the start, the wrapper process stays, the CLI spawn count stays 0, and the next hello is the new target; an uninstalled config still spawns detached and survives the parent exit.

## 4. Docs and upload argv

- [x] 4.1 Update `docs/serial.md` and `docs/integration.md`: `daemon release` and plain `daemon stop` free the port and leave the process, `daemon disable` stays down until `daemon start`, `daemon stop --yield` keeps the process for 180s and does not reopen the port, and `daemon install` is the per-user login wrapper. State that a live process is assured only after install. Verify the docs name `release`, `disable`, `stop --yield`, and `install`, and do not tell users to install the CLI from a git URL.
- [x] 4.2 Keep `scripts/park_servos_before_upload.py` on `daemon stop --yield` after park. Verify the existing argv check still shows park then `daemon stop --yield`.

## 5. Integration check

- [x] 5.1 Run `npm test --prefix packages/tiny-engineer-cli` and fix failures until green.

## Workflow follow-up

- Archive the change after review so the `cli-host-daemon`, `outbound-events`, `servos-park`, and `cli-transport` deltas merge.
