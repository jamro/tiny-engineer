# Design

## Context

See proposal.md for why. Today every serial command calls `connectSerialRobot` in its own process (`packages/tiny-engineer-cli/src/robot/serial/client.js`): open, `sync`, use, close. `watch` and `monitor` stay open until Ctrl+C. `SerialRobotClient` already demuxes response ids and has separate event and log handlers, but only inside one process.

Users run the published npm package (`npx -y tiny-engineer` or a global install). Contributors also run `node packages/tiny-engineer-cli/bin/tiny-engineer.js`. Runtime files already go through `userConfigDir()` (`TINY_ENGINEER_CONFIG_DIR`, XDG, Windows `APPDATA`). Pre-upload park is `scripts/park_servos_before_upload.py`. Hook timeout is 30s. The USB frame format does not change.

## Goals / Non-Goals

**Goals:**

- One process owns the serial device and serves every serial CLI entry point.
- The process shell (lifecycle, IPC, replace rules) stays independent of the serial link so later methods can be added beside it.
- npm installs and a repo checkout of `packages/tiny-engineer-cli` both work on the same socket.
- Flash can still open the CDC port.

**Non-Goals:**

- Proxying HTTP or WebSocket through the daemon.
- Button automations, a menu-bar app, or more than one robot.
- An OS service (launchd, systemd, Windows service) or a plugin loader.
- Firmware or serial-protocol changes.
- Opening the tty directly from short-lived commands as a fallback when the daemon is up.

## Decisions

### D1: Same binary, subcommand `daemon`

`tiny-engineer daemon start|stop|status|restart`. Auto-start spawns `process.execPath` plus the running script path (`process.argv[1]`) and `daemon`. That is the npm cache entry, the global prefix, or `packages/tiny-engineer-cli/bin/tiny-engineer.js`. Never a git URL.

The child is detached, `unref`'d, and ignores SIGHUP, so closing the terminal that triggered auto-start does not kill it. Stdio is not inherited. Diagnostics append to `daemon.log` in the user config directory, capped at about 1 MiB by truncation.

**Alternatives:** A second npm package (rejected — two versions to publish). Idle-exit (rejected — this change is the stay-until-stop lifetime).

### D2: IPC and files

Newline-delimited JSON over `node:net`. POSIX: unix socket `daemon.sock` mode `0600`. Windows: a named pipe under the same user config directory. No extra dependency.

`daemon.json` records pid, proto, version, channel, package root, source hash, and the open port path. `daemon.yield` stores an expiry timestamp. `daemon.lock` is an exclusive create (`wx`) so two replacers cannot both spawn.

`hello` is the first line the server writes. `daemon.stop` accepts `reason` `stop` or `replace`. Those two messages stay valid when `proto` increases so a newer client can still shut an older daemon down. Subscribers get `bye` with `reason` `stop` or `replace` before the socket closes.

Other methods (`robot.request`, `subscribe`, `play`) may change only with a proto bump.

**Alternatives:** HTTP on localhost (rejected — another port and a second stack for a local call). Stuffing WAV bytes into JSON (rejected — `play` already takes a file path; the daemon reads that path).

### D3: Serial link is a module the shell calls

The daemon holds one `SerialRobotClient`. Socket handlers call `request` / `play`. Event and log callbacks fan out to subscribers. The client already allocates wire ids, so socket clients do not share an id space on the USB link.

The first successful open chooses the port (flag, env, cache, autodetect — same order as today). A later command whose port path differs gets an error naming both paths. `daemon restart --port <path>` stops and starts so the new process can open that path.

Cable unplug does not exit the daemon. The link is marked down; the next request opens again. `watch` prints one error and keeps its subscription.

### D4: Who replaces whom

Channel is `npm` when the real package root path contains `node_modules/tiny-engineer` (global install and `npx` cache). Otherwise it is `dev`.

Source identity is a hash of `bin/tiny-engineer.js` and `src/**/*.js` at process start. Compare SemVer from that install's `package.json`.

| Caller | Daemon | Action |
| --- | --- | --- |
| npm, higher SemVer | npm | replace from the caller |
| npm, same SemVer | npm | keep, including a different `npx` cache path |
| npm, lower SemVer, same proto | npm | keep |
| npm, any version | dev | keep |
| dev | npm | replace from the checkout, even if its version is lower |
| dev, same package root, hash changed | dev | replace |
| dev, different package root | dev | keep; user runs `daemon restart` from the tree they want |
| client proto unknown | any | error, do not stop |
| client proto newer | any | `daemon.stop` reason `replace`, then spawn the caller |

If a serial play is in progress and the caller still understands the running proto, serve the call on the current daemon and do not stop it. The next command after play re-evaluates the table. If the caller does not understand the proto, fail that command and leave playback running.

One exclusive lock covers replace. The loser waits for the new hello.

**Alternatives:** Always restart when the script path differs (rejected — `npx` cache paths flap). mtime instead of a content hash (rejected — checkout timestamps flap; npm extracts of one version hash the same). Let a newer npm package replace a dev daemon (rejected — the checkout's `package.json` often stays behind the registry, so the local CLI could not take the daemon back).

### D5: Yield for upload

`daemon stop` releases the port and does not write a yield. `daemon stop --yield` writes `daemon.yield` for 180 seconds, then exits. Auto-start refuses while the file's expiry is in the future and returns a distinct error. `daemon start` deletes the file and starts.

`scripts/park_servos_before_upload.py` keeps the park invocation, then runs `daemon stop --yield` with the same CLI resolution (PATH `tiny-engineer`, else the repo `bin/tiny-engineer.js`). Either failure warns and upload continues.

`watch` and `monitor` on `bye` reason `stop` exit and do not spawn. On `replace` they wait and resubscribe. On a socket drop with no `bye` they try once, then exit non-zero.

### D6: Wedged or foreign pid

Connect and require a hello within a short timeout. Dead pid or a live pid whose command line is not this daemon: delete the stale files, do not signal, start. Live daemon pid and no hello: SIGTERM, wait, SIGKILL, then start. Two auto-starts: the bind loser connects to the winner.

## Risks / Trade-offs

- [Cold `npx` plus daemon start plus serial `sync` in one hook] → Spawn is local; `sync` is the same 1.5s probe as today. Hook timeout stays 30s. Fail-open behavior of hooks stays.
- [Contributor machine: npm hooks talk to the dev daemon] → `daemon status` shows `channel=dev` and the package root. `daemon stop` lets the next `npx` command start an npm daemon.
- [Flash longer than 180s, then a serial hook auto-starts and steals CDC] → Yield covers a normal upload. A hook during an unusually long flash can still grab the port. Accepted; raising the window would block intentional `anim` after a cancelled upload for longer.
- [CLI without this change runs while a new daemon holds the port] → That process gets a busy port. Updating the CLI (npm or the checkout) is the fix. `daemon stop` frees the port for old tools.
- [Windows command-line check for a foreign pid is weaker than `ps`] → If the command line cannot be read, treat a live unknown pid as foreign (do not kill) and fail the command with a message to run `daemon stop`. Do not SIGKILL a process we cannot identify.

## Migration Plan

No config migration. The next serial command after upgrade starts the daemon. Rollback is `daemon stop` and installing the previous CLI; the port is free once the process exits. Pre-upload must ship in the same change as the daemon, or upload breaks whenever a daemon is running.

No open questions. Replace-during-play, yield length, and channel priority are fixed above so the specs stay as written.
