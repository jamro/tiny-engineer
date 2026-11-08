# Proposal

## Why

When the host-daemon IPC socket closes mid-request (daemon crash, replace, disable), pending `createPeer().request()` waiters only fail after their timeout — up to 11 minutes for `play`. Callers hang with no signal that the peer is gone. Serial already rejects pending ops on port close; NDJSON IPC should match.

## What Changes

- On socket `close` or `error`, reject every outstanding IPC `request()` waiter immediately with a clear connection-closed error (mirror `SerialRobotClient` → `"serial port closed"`).
- Keep existing per-request timeouts for the alive-but-silent case.
- No auto-retry, no `sourceHash` cache, no play/end protocol changes.
- Add a focused unit/integration test that a mid-flight request fails fast when the socket dies.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `cli-host-daemon`: Require that an in-flight client IPC request fails promptly when the daemon connection closes, instead of waiting solely for the request timeout.

## Impact

- Code: `packages/tiny-engineer-cli/src/daemon/ipc.js` (`createPeer`); tests under `packages/tiny-engineer-cli/test/`.
- Callers of `peer.request` (`rpc.js` play/anim, ensure stop, status, etc.) surface a connection error sooner; message string is new but behavior is fail-faster, not a protocol break.
- Scope for commits: `integrations` (CLI under `packages/`).
