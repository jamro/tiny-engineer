# Tasks

## 1. Replace policy

- [x] 1.1 Add a pure helper that classifies a package root as `npm` or `dev` (`node_modules/tiny-engineer` vs `packages/tiny-engineer-cli` outside `node_modules`), hashes `bin/tiny-engineer.js` plus `src/**/*.js`, and decides replace vs keep from the design table (SemVer, channel, same root, proto). Verify with unit tests for: higher npm replaces npm, same SemVer different cache path keeps, lower npm does not downgrade, dev replaces npm even at a lower version, npm does not replace dev, same dev root with a changed hash replaces, a different dev root does not, unknown proto does not stop, newer proto does.
- [x] 1.2 Verify the helper reads version from the install's `package.json` and does not treat a git URL as an install path (test passes a `node_modules/tiny-engineer` path and a repo `packages/tiny-engineer-cli` path).

## 2. Daemon process and IPC

- [x] 2.1 Add `tiny-engineer daemon start|stop|status|restart` in `packages/tiny-engineer-cli/src/cli.js`, spawning `process.execPath` and the running script with detached/`unref` and SIGHUP ignored. Verify `start` while running and `stop` while absent both exit 0, and a parent exit does not kill the daemon (`daemon status` from the test still sees it).
- [x] 2.2 Serve newline-delimited JSON on a `0600` unix socket (named pipe on Windows) under `userConfigDir()`, with `hello` first and `daemon.stop` (`reason` `stop` or `replace`). Persist `daemon.json`. Verify `TINY_ENGINEER_CONFIG_DIR` keeps the endpoint inside that directory and `daemon status` prints pid, version, channel, package root, proto, and port state (or not running).
- [x] 2.3 Implement yield: `daemon stop --yield` writes a 180s expiry and releases the port; plain `daemon stop` does not; auto-start during yield fails with a distinct error and does not spawn; `daemon start` clears the yield. Verify with tests on a temp config dir.
- [x] 2.4 Single-flight replace lock, one auto-start attempt per command, foreign-pid handling (do not signal; if the command line cannot be identified, fail and tell the user to run `daemon stop`), and wedged-daemon SIGTERM then SIGKILL. Verify two concurrent starts leave one process, a dead pid is replaced without signaling, and a second hello failure in one command exits non-zero.
- [x] 2.5 Cap `daemon.log` near 1 MiB by truncation. Verify a test or a length check after a forced write.

## 3. Serial link inside the daemon

- [x] 3.1 Route serial `robot.request`, event/log subscribe, and `play` (filesystem WAV path, daemon reads the file) through one `SerialRobotClient`. Fan out events and logs to every subscriber. Keep the port open when a client disconnects. On stream failure, still best-effort `play_end` without dropping the tty. Verify with the existing injectable port fake: two clients get events, and a failed play attempts `play_end` while the fake port stays open.
- [x] 3.2 Keep the first opened port. A different `--port` fails and names both paths. `daemon restart --port` stops and lets the new process open the requested path. Cable-unplug marks the link down without exiting; the next request opens again. Verify with unit tests. Defer replace while a play is active when the caller understands the proto; the following command still applies the replace table. Verify a play-active fake does not receive stop before play ends.

## 4. CLI serial commands

- [x] 4.1 Send serial `anim`, `play`, settings, test, servo, park, health, reset, and serial doctor/hooks through the daemon (auto-start). HTTP paths MUST NOT spawn it or open a serial device. Verify tests: serial anim with a running fake daemon does not open a second port; HTTP anim does not create an endpoint; a serial hook-style call succeeds while a subscriber is connected.
- [x] 4.2 Point `watch` and `monitor` at subscribe. On `bye` `stop`, exit and do not spawn. On `bye` `replace`, resubscribe. On a drop without `bye`, retry once then exit non-zero. Initial link failure still exits non-zero. Serial `watch` still prints only `Evt` lines and does not need a URL. Verify with tests for those four exits plus an anim during an active watch subscription.
- [x] 4.3 Update CLI help for `daemon` and the serial ownership note. Verify `tiny-engineer daemon --help` and `tiny-engineer --help` mention start/stop/status/restart and `--yield`.

## 5. Upload and docs

- [x] 5.1 After the park attempt in `scripts/park_servos_before_upload.py`, run `daemon stop --yield` with the same CLI resolution (PATH, else repo `bin/tiny-engineer.js`). Park failure and stop failure still warn and continue. Verify a test or a dry run of the script's argv shows park then `daemon stop --yield`.
- [x] 5.2 Update `docs/serial.md` and `docs/integration.md`: daemon lifetime, auto-start, npm vs `packages/tiny-engineer-cli` dev channel, `daemon stop` before other serial tools, and the upload yield. Verify the docs name `daemon stop --yield` and do not tell users to install the CLI from a git URL.

## 6. Integration check

- [x] 6.1 Run `npm test --prefix packages/tiny-engineer-cli` and fix failures until green.

## Workflow follow-up

- Archive the change after review so `cli-host-daemon` lands in main specs and the `cli-transport`, `outbound-events`, and `servos-park` deltas merge.
