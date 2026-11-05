# Proposal

## Why

When the CLI must replace a running host daemon (newer protocol, newer npm version, or a changed dev checkout), an active serial `play` currently blocks that replace. The client then keeps using the old process with no clear signal (`ensureDaemon` returns the stale connection). That mismatches the intent of replace and leaves upgrades stuck behind long WAV playback. Prefer cutting play for replace: rare host upgrades beat finishing an arbitrary clip.

## What Changes

- **BREAKING** (behavior vs current `cli-host-daemon` requirement): a daemon **replace** MAY abort an in-flight serial `play`. The process exits, closes the serial link, and the replacing client starts a new daemon and completes its command.
- Remove the server-side `play-active` refusal on `daemon.stop` with `reason=replace`.
- Remove the client `ensureDaemon` path that treated `play-active` as success with the old connection.
- Align `daemon install` with the same rule: do not refuse install solely because play is active; a live daemon is replaced (play aborted) so the supervisor can take ownership.
- Keep **plain `daemon release`** (without force/yield) refusing while play is active — port yield for casual release stays protected.
- In-flight `play` IPC callers fail when the link/process goes down (expected); no new pending-replace queue.
- Update tests and serial/integration docs that describe replace waiting on play.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `cli-host-daemon`: Replace the “do not cut play” contract with “replace aborts play and proceeds”; keep non-force release blocked during play; clarify install during play follows replace abort semantics.

## Impact

- Code: `packages/tiny-engineer-cli/src/daemon/server.js`, `ensure.js`, `install.js` / `commands.js` install play check, related tests (`daemon.test.js`, supervisor install-during-play), docs if they claim replace waits for play.
- No firmware change required; serial drop ends the stream on device as today when the host closes the port.
- Scope for commits: `integrations` (CLI under `packages/`).
