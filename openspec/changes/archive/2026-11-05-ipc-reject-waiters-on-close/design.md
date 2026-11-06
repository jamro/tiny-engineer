# Design

## Context

See proposal.md — Why. `createPeer` in `packages/tiny-engineer-cli/src/daemon/ipc.js` maps request ids to resolve-only waiters and rejects only on `setTimeout`. `subscribeOnce` already handles socket `close`/`error`. `SerialRobotClient.close()` rejects `_pending` with `"serial port closed"`. Same peer helper is used on the daemon server side, but the server does not call `peer.request()`.

## Goals / Non-Goals

**Goals:**

- Fail every in-flight `request()` as soon as the socket closes or errors.
- Preserve timeout behavior when the socket stays up.
- Settle each waiter at most once (no double reject).

**Non-Goals:**

- Auto-retry after close; `sourceHash` caching; changing play/replace protocol; renaming IPC methods; changing default timeout values.

## Decisions

### 1. Reject waiters inside `createPeer` on `close` and `error`

Store `{ resolve, reject, timer }` (or equivalent) per id. On `close`/`error`, clear timers, reject all with `new Error("daemon connection closed")`, clear the map, set a closed flag so late messages cannot resolve. Mirror serial’s `"serial port closed"` wording for host IPC.

**Why not only document longer timeouts:** hang is the bug; timeouts are a backstop for a live silent peer.

**Why not handle close only in `daemonPlay` / `rpc.js`:** every `request()` caller shares the gap; fix once at the peer.

### 2. Keep request timeouts unchanged

Default 5s and long play timeout stay. Close path is additive.

### 3. No server-side special case

Server `createPeer` gets the same listeners; empty waiter map → no-op. Prefer one code path over client-only wiring.

### 4. Tests

Unit-style: create peer over a local socket (or mock socket), start `request` with a long timeout, destroy the remote end, assert reject well under the timeout and message matches. Optional: assert a follow-up request on a closed peer fails immediately if that API is exposed; otherwise out of scope.

## Risks / Trade-offs

- [Caller `finally { socket.end() }` racing a late response] → Today request completes before `end` in `rpc.js`; if a caller ends early, fail-fast is correct.
- [Half-close / error then close double-fires] → Settled flag / cleared map prevents double reject.
- [Callers matching timeout error strings only] → New error text; prefer matching `/connection closed|timed out/` if needed — no known hard dependency on timeout-only.

## Migration Plan

1. Ship with CLI package; no on-disk migration.
2. Rollback: revert `ipc.js` (+ test).

## Open Questions

None.
