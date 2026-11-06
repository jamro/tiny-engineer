# Design

## Context

See proposal.md — Why. Today only `daemon.stop` with `reason=replace` refuses while `link.playActive` (`server.js`). Non-replace stop, SIGTERM, and `link.close()` during shutdown already tear down the serial session mid-play. `ensureDaemon` treats a `play-active` error as success and returns the old connection — the silent stale-daemon bug. `installSupervisor` additionally throws if `playActive` before calling replace shutdown.

## Goals / Non-Goals

**Goals:**

- Replace always proceeds: ack stop, shutdown (bye + `link.close()`), client obtains a new daemon.
- Install during play uses the same replace path (no special refuse).
- Keep non-force `daemon.release` blocked during play.

**Non-Goals:**

- Pending-replace queue, client wait-until-play-ends, or fail-and-retry-only policy.
- Changing firmware stream abort protocol beyond “host closed the port”.
- Allowing plain release to cut play without `--yield` / force.
- IPC method renames.

## Decisions

### 1. Cut on replace only; reuse existing shutdown

Delete the `reason === "replace" && link.playActive` early return in `server.js`. Flow becomes: reply `{ ok: true }` → `shutdown("replace")` → bye subscribers → `link.close()` → exit. In-process `link.play()` ends when the serial client closes; IPC `play` waiters see connection loss / error. No separate `play_end` orchestration required for correctness (nice-to-have later, not in this change).

**Why not hard `process.exit` without close:** shutdown already closes the link; keep that path so watchers get `bye` and the tty is released cleanly for the next daemon.

### 2. Delete ensure’s `play-active` success branch

In `ensure.js`, after `daemon.stop` reason replace, never `return conn` on `play-active`. That error should no longer occur; if it does (old daemon binary), treat as failure or retry once after a short wait — prefer **throw** so we do not revive silent keep. Simplest: assert/throw if `play-active` still returned (forces both sides updated together).

### 3. Install: drop playActive pre-check

Remove `if (opts.playActive) throw` in `installSupervisor`. Remove the post-shutdown `play-active` throw (dead after server change). `commands.js` can stop probing `playActive` solely to pass into install (optional cleanup). Install then always attempts `shutdownDaemon(..., "replace")` when a live socket exists.

### 4. Release protection unchanged

`link.release(false)` still returns false while `playActive`; server still answers `play-active`. Yield/force still force-close.

### 5. Tests and docs

- Rewrite `replace waits while play is active` → replace during play aborts and new daemon serves.
- Update `newer proto is served during play…` if it assumed old proto survives; expect replace to complete mid-play instead.
- Update install-during-play tests to expect success + abort.
- Docs (`docs/serial.md` / integration) only if they claim replace waits for play (grep); prefer release wording already fixed in prior change.

## Risks / Trade-offs

- [Urwane audio / failed `play` command] → Accept for rare replace; document in spec migration note.
- [Race: play finishes between stop ack and close] → Harmless; shutdown still exits.
- [Old CLI against new daemon or vice versa during rollout] → New server never returns play-active on replace; old ensure that returned conn on play-active becomes dead code path — remove it so mixed versions fail closed rather than silent keep.
- [Two replaces concurrent] → Existing lock/`obtain` rules still serialize starts.

## Migration Plan

1. Ship server + ensure + install together in one CLI release.
2. No on-disk format migration.
3. Rollback: revert; old “refuse replace during play” returns (and silent ensure bug returns with it).

## Open Questions

None. Soft `play_end` before close deferred.
