# Design

## Context

See proposal.md for why reactions live in the existing host daemon.

Today `serveDaemon` (`packages/tiny-engineer-cli/src/daemon/server.js`) owns a lazy serial link (`link.js`). The port opens on the first serial command, not at `daemon start`. `eventSubs` fans `Evt` frames to IPC subscribers (`watch`, `monitor`). HTTP robot commands never enter this process. `SIGHUP` is registered and empty. The daemon does not load the user `env` file into `process.env`. The login wrapper restarts a daemon that has accepted a hello; that restart is a new process.

`watch.js` already builds `ws://<host>:81/events` from an `http:` base URL and redacts the token. Reactions reuse that, inside the daemon.

## Goals / Non-Goals

**Goals:**

- One process holds the reaction link and the rule runner.
- Serial `Evt` frames and the HTTP event socket never both feed the runner.
- A child program cannot stall the serial read loop or an in-flight `play`.
- No rules file means the port and WebSocket behavior of today's daemon.

**Non-Goals:**

- Proxying HTTP `anim` or hooks through the daemon.
- Moving HTTP `watch` into the daemon (it keeps its own socket).
- Shell rules, event-to-argv templates, in-process JS actions, webhook actions, per-repo rule files, more than one robot.

## Decisions

### Runner sits beside the serial link, not on `eventSubs`

`eventSubs` stays the fanout for IPC subscribers. The runner has its own listener.

- User-config transport `serial`: that listener is the serial event handler. No WebSocket.
- User-config transport `http`: that listener is one reconnecting WebSocket. Serial `Evt` frames, including those from a one-off `--transport serial` command, do not enter the matcher.
- Switch: close the old listener, then open the new one. Never both.

Alternative: subscribe the runner through the same `eventSubs` set as `watch`. Rejected. A serial command while config is `http` would run every rule a second time, because the device emits the event on both paths.

### Rules are data, spawn is the only action

New modules under `src/daemon/`:

- `rules.js` — parse, keep last good set, match `type` / optional `source`, ignore envelope `v` other than 1. No I/O besides reading the file.
- `actions.js` — spawn, per-rule slot, global cap of 8, timeout, stderr drain.
- `reactions.js` — which link is live, suspend latch, calls match then spawn. Does not await the child.

`rules.json` path goes in `daemon/paths.js` next to the other config-dir files. The CLI writes it by temp file plus rename, mode `0600` on POSIX, so a reload cannot see a half-written file.

`SIGHUP` re-reads `rules.json` and the user-config transport. Same transport does not close the serial port. `rules add` / `rules remove` send `SIGHUP` to the pid in `daemon.json` when the process is up. They do not call `daemon install`.

Alternative: poll the file. Rejected. A poll races a partial write, and setup already has an explicit signal.

### Child environment is an allowlist

`spawn(program, args)` with `shell: false`. Stdin is one JSON line then end. Stdout is ignored. Stderr is read and capped so a full pipe cannot block the child. Working directory is the user home directory.

The child env is built, not inherited wholesale: `HOME`, `PATH`, `USER`, `LANG`, `TMPDIR`, plus `TINY_ENGINEER_EVENT_TYPE`, `TINY_ENGINEER_EVENT_SOURCE`, and `TINY_ENGINEER_EVENT_MS`. On Windows add the variables Node needs to start (`SystemRoot`, `COMSPEC`). The access token is read from the user env file only when opening the WebSocket and is never copied into `process.env` or the child env.

`rules add` resolves a relative program name with the invoker's `PATH` and stores the absolute path. The daemon does not search `PATH`. The login agent PATH is only `/usr/bin:/bin` (and the Windows equivalent), so a stored name like `node` would break after install.

The child is started in its own process group (`detached: true` on POSIX) and is not unref'd. Timeout and daemon SIGTERM kill the group (`process.kill(-pid)`). On Windows, kill the tree with `taskkill /pid /t /f`. SIGKILL of the daemon itself can orphan children; the wrapper will not reap them.

Alternative: `sh -c` as the default. Rejected. Event text would become a shell injection, and quoting differs per OS.

### Suspend latch is a file, not only a flag

`daemon release` and `daemon stop` without `--yield` already close the port and leave the process up. If the runner then called `ensure()` on a timer, release would race esptool.

The runner must not call `ensure()` while a suspend file is present (`daemon.reactions-suspended` in the config dir, mode `0600`). Release and stop write it only after the port release succeeds (a release refused because play is active does not suspend). `daemon start` and `daemon restart` remove it, then open the port when a serial rule is active. A serial command that opens the port removes it too, so later `Evt` frames reach the runner again. Yield stays the existing `daemon.yield` file: the runner treats yield like suspend and does not open.

The file, not a variable, is what makes a wrapper restart honor release. The wrapper starts a new process; an in-memory flag would forget the release and grab the USB port again.

Alternative: in-memory flag only. Rejected for that crash window. Alternative: reuse the yield file for every release. Rejected. Plain release must not create a 180s yield; the spec already says so.

HTTP reactions ignore the suspend file and the yield file. `daemon disable` exits the process, which drops the socket. The supervisor already refuses to start again until `daemon start`.

### Status is three extra fields on the existing report

`daemon.status` gains `reactionLink` (`off`, `up`, `retrying`, `suspended`, plus the port path or redacted WebSocket URL when up), `rules` (count and file error string or null), and `lastAction` (`none`, `exit`, `timeout`, `skipped`, with type and program). `daemon status` prints them. Token values never appear.

### HTTP control plane stays in `transport.js`

No change to the "HTTP commands do not start the daemon" path. A rule whose program is `tiny-engineer anim` is an ordinary child. On serial it re-enters this daemon over IPC. That is safe only because the runner does not wait for the child. During `play` the existing play lock still applies; this design does not add an `anim` action kind to bypass it.

## Risks / Trade-offs

- [Armed serial `daemon start` takes the USB port] → No rules file keeps today's closed-port start. Document the change in `docs/serial.md` and the daemon help text.
- [Release latch forgotten across a crash if it were memory-only] → Persist `daemon.reactions-suspended`.
- [Login-agent PATH is minimal] → Store an absolute program path at `rules add`. Scripts that use `sh -c` still see the agent's PATH; docs say to use absolute paths inside those scripts.
- [Node is one thread] → Matcher is synchronous and tiny. The runner never awaits a child. An in-process action would void this; it is a non-goal.
- [WebSocket cap of 4 clients] → The daemon holds one slot whenever HTTP reactions are armed. `watch` still opens a second.
- [SIGKILL orphans grandchildren] → Accepted. SIGTERM path kills the group. Wrapper restart does not hunt orphans.
- [Windows has no POSIX process group] → `taskkill /t`. Same timeout and slot rules.
- [Token rotation] → Re-read the user env file on every WebSocket attempt. Log the redacted URL only.
- [Bad `rules.json` while serial hooks are in use] → Keep the last good set so a broken edit does not unload the daemon. First boot with a bad file is zero rules, process still up.

## Migration Plan

No firmware and no existing file to migrate. A missing `rules.json` is zero rules, so shipping the daemon change does not take the USB port or open port 81 for current users.

Rollback is a CLI downgrade. An old daemon ignores `rules.json` and the suspend file. `daemon start` once again leaves the port closed. Rules already written remain on disk for a later upgrade.

Deploy is the normal CLI publish. Docs to update in the same change: daemon help, `docs/serial.md`, `docs/integration.md`.
