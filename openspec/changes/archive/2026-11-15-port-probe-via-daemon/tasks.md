# Tasks

## 1. Daemon probe IPC

- [x] 1.1 Add a cache-skip open+sync path used by probe (flag or helper beside `connectSerialRobot`) and verify a successful open does not call `writePortCache` (unit test or existing cache test adapted)
- [x] 1.2 Implement `robot.probe` on the daemon (`link` + `server`): ordered candidates, sync-only when session matches, skip other paths when held, walk with cache-skip open when free, refuse when play active; verify with daemon tests covering free-walk, held-skip, and play-refuse
- [x] 1.3 Expose a client helper (e.g. `daemonProbe` / `serialProbe`) that auto-starts like other serial RPCs and returns per-candidate results; verify disable/yield fail before open (reuse existing ensure patterns / tests)

## 2. Port command

- [x] 2.1 Change `runPort(--probe)` to build the same candidate order as today, call the daemon probe helper, print per-path lines (ok / fail / skipped-held), and remove in-process `SerialRobotClient` open for probe; verify `port-command.test.js` updated mocks/assertions and CLI no longer opens tty on probe
- [x] 2.2 Keep plain `port` list/score unchanged; verify existing formatPortScan tests still pass

## 3. Docs and suite

- [x] 3.1 Update `docs/serial.md` for probe-via-daemon, no cache write, may leave port open, disable/yield; verify wording matches specs
- [x] 3.2 Run `npm test --prefix packages/tiny-engineer-cli` and fix regressions

## Workflow follow-up

- After apply + verify: `/opsx-archive` (sync delta specs into main `cli-transport` / `cli-host-daemon`)
