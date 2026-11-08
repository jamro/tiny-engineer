# Design

## Context

See proposal.md — Why. Today `runPort(--probe)` builds a candidate list then `openSynced(SerialRobotClient)` in-process. Daemon `link.ensure` already refuses switching away from an open session (`daemon is on X, this command asked for Y`). `connectSerialRobot` writes the port cache on successful open — probe must not use that path as-is.

## Goals / Non-Goals

**Goals:**

- One USB owner: probe sync only inside the daemon.
- Preserve list/score UX and no-cache-write for probe.
- Safe with live `watch` / reactions (no yank of held session).

**Non-Goals:**

- Changing plain `port` listing.
- Option B (gate-only) or removing `--probe`.
- Multi-port simultaneous opens (USB exclusive).
- Protocol SemVer bump unless an existing client compatibility gate requires it (prefer additive RPC method).

## Decisions

### 1. New IPC method `robot.probe` (not a loop of `robot.request` sync)

**Choice:** Client sends `{ method: "robot.probe", params: { candidates: string[] } }`. Daemon returns per-path results plus overall ok.

**Why:** `robot.request` + `ensure(port)` cannot walk candidates while holding another path; a dedicated method owns the “held vs free” policy and cache-skip open path.

**Alternatives:** CLI loops `serialRequest("sync", {}, { port })` — fails when held on different path; ambiguous cache writes via `connectSerialRobot`. Rejected.

### 2. Candidate list stays in the CLI

**Choice:** CLI still `SerialPort.list` + score + env/cache order (same as today), then passes ordered paths to the daemon.

**Why:** Listing is not exclusive ownership; keeps scoring rules in one place (`scoreSerialPort`); daemon stays thin.

### 3. Held session: sync only, no switch

**Choice:** If `link` has session on path P: for candidates equal to P, sync; for others, result `skipped` (daemon holds P). Never `release` to try B during probe.

**Why:** Yanking would `link-down` watch/reactions. Spec requires this.

### 4. Free session: walk with open that skips cache write

**Choice:** Add a connect/open helper used only by probe (or a flag on connect) that open+syncs without `writePortCache`. On first success, keep session (same as post-anim). Failed candidate: close that attempt, try next.

**Why:** Matches “probe does not write cache” and daemon ownership after success.

### 5. Lifecycle: auto-start, disable, yield, play

**Choice:** Probe goes through the same ensure/auto-start client path as other serial ops. Disable/yield → fail before open. Play active → daemon refuses probe.

**Why:** Spec parity with hooks/doctor/anim.

### 6. Docs

**Choice:** Update `docs/serial.md` one paragraph: probe via daemon; may leave port open; still no cache write; yield/disable apply.

## Risks / Trade-offs

- [Behavior change: port stays open after successful probe] → Document; aligns with daemon model; flash still needs `daemon release` / `--yield`.
- [Probe of “other” paths skipped while held] → Print clear skip reason so users are not surprised.
- [Cache-skip open duplicates connect logic] → Prefer small flag/helper next to `connectSerialRobot` rather than copy-paste sync retries.
- [IPC clients on old daemon] → Auto-replace / protocol rules already replace incompatible daemons; additive method on current protocol if hello stays compatible.

## Migration Plan

Ship in CLI package; no firmware change. Users on serial: `port --probe` with live daemon should start working instead of busy-fail. Rollback = revert package.

## Open Questions

None for scope — option A locked; held-port skip policy fixed above.
