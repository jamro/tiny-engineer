# Tasks

## 1. Rules file

- [x] 1.1 Add `rules.json` to `daemon/paths.js` (same config dir as `env`, including `TINY_ENGINEER_CONFIG_DIR`) and verify a unit test resolves that path inside an isolated config dir and not the default one
- [x] 1.2 Implement `daemon/rules.js` load and match: `v` 1, `on` plus non-empty string `run`, optional `source` and positive `timeoutMs`, ignore other fields, keep the last good set, empty `rules` clears the file error, envelope `v` other than 1 matches nothing. Verify `node --test test/rules.test.js` from `packages/tiny-engineer-cli`

## 2. Child processes

- [x] 2.1 Implement `daemon/actions.js`: `shell: false`, event JSON line on stdin, allowlisted env with `TINY_ENGINEER_EVENT_TYPE`, `TINY_ENGINEER_EVENT_SOURCE`, and `TINY_ENGINEER_EVENT_MS`, no access token, cwd = home, one live child per rule, global cap 8, skip reported, default timeout 30000 and `timeoutMs` override, stderr drained, POSIX process-group kill and Windows `taskkill /t`. Verify `node --test test/actions.test.js` covers spawn args, token absence, skip, cap, and timeout using a fake spawn or a short fixture script

## 3. Reaction link inside the daemon

- [x] 3.1 Add `daemon/reactions.js` and call it from `serveDaemon`. Serial user-config transport listens on the serial event path and does not open a WebSocket. HTTP user-config transport uses `eventsWebSocketUrl` from `commands/watch.js`, reconnects, re-reads the token from the user env file each attempt, redacts it in logs, and rejects `https:` bases. A `--transport` flag on one command MUST NOT move this link, and a serial `Evt` during HTTP reactions MUST NOT start a rule. Verify with daemon tests that one published event starts each matching rule once
- [x] 3.2 Zero active rules MUST NOT open a WebSocket and MUST NOT call serial `ensure` for reactions. Verify a daemon test with a missing `rules.json` leaves the port closed until a serial command
- [x] 3.3 Write `daemon.reactions-suspended` (mode `0600`) only after `daemon release` or `daemon stop` without `--yield` succeeds. Do not write it when release fails because play is active. The runner MUST NOT call `ensure` while that file or `daemon.yield` is present. `daemon start` and `daemon restart` remove the suspend file and, when a serial rule is active, open the port. A serial command that opens the port removes the suspend file and later event frames reach rules. HTTP reaction sockets stay up across release, stop, and yield. Verify these cases in `test/daemon.test.js` (or `test/reactions.test.js` if the daemon file is already too large)
- [x] 3.4 Handle `SIGHUP` by reloading `rules.json`. Same transport MUST NOT close and reopen the serial port. A transport change MUST close the previous reaction link before opening the next. Invalid JSON keeps the previous set and records a file error; the daemon stays up. Verify with a test that adds a rule on an open port and a test that switches `http` to `serial`
- [x] 3.5 On SIGTERM, kill reaction children. A serial `anim` issued while a child is still running MUST complete without waiting for that child. Verify both with a daemon test
- [x] 3.6 Extend `daemon.status` and the `daemon status` text with reaction link (`off`, `up`, `retrying`, `suspended`, plus port path or redacted URL), rule count, rules file error, and last action (`none`, `exit`, `timeout`, `skipped`). Update `daemonHelp` so start/restart/release/stop describe the armed-serial port contract. Verify a status test when running and when stopped, and that the help text mentions the armed-serial open and the suspend latch
- [x] 3.7 Update `docs/serial.md` and `docs/integration.md` for armed serial start, release/stop suspend, yield, and HTTP reactions surviving release. Verify the new sentences match the help text from 3.6 and do not tell HTTP `anim` to go through the daemon

## 4. Rules CLI

- [x] 4.1 Add `rules list`, `rules add <type> -- <program> [args...]`, and `rules remove <index>` in `src/commands/rules.js`, register them in `src/cli.js`, and show them in the top-level help. Add writes `rules.json` via temp file plus rename, mode `0600` on POSIX, stores a resolved absolute program path, rejects an unresolvable program without changing the file, does not run `daemon install`, and sends `SIGHUP` when `daemon.json` has a live pid. Remove uses the 1-based index from list, fails cleanly on a bad index, and reloads the same way. Verify `node --test test/rules-command.test.js` for path resolution, mode, no install, bad index, and reload of a live daemon
- [x] 4.2 Document `rules add` / `list` / `remove` in the CLI help and in `docs/integration.md`, including that programs are stored absolute and that `sh -c` scripts still see the login agent's PATH. Verify the documented add command matches the parser used by 4.1

## 5. Package check

- [x] 5.1 Run `npm test --prefix packages/tiny-engineer-cli` and verify the suite passes, including existing daemon, watch, and HTTP-transport tests
