# Proposal

## Why

Host-daemon layering still has two structural debts from the architecture review: (5a) `daemon/reactions.js` imports WebSocket URL helpers from `commands/watch.js` (server → CLI command), and (5c) `robot/transport.js` duplicates the HTTP-vs-serial fork that `robot/ops.js` already centralizes in `withTransport`. Neither changes user-visible robot behavior today, but both raise cognitive load and make the next serial/HTTP feature easy to get wrong. **Out of scope:** `port --probe` (5b) — separate behavior change.

## What Changes

- Move `eventsWebSocketUrl` / `redactEventsUrl` out of `commands/watch.js` into a shared module (e.g. `robot/events-url.js`); `watch` and `reactions` both import from there.
- Export (or extract) a single transport helper and route `anim` / `play` / `health` / `robotOp` through it so serial always goes via `daemon/robot.js` and HTTP via existing HTTP helpers — same outcomes as today.
- Update imports/tests; no daemon protocol, HTTP API, or CLI flag changes.
- Specs skipped (`skip_specs: true`): pure structure; `cli-transport` already requires serial through the host daemon and day-to-day parity.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- (none — structural refactor only; `skip_specs: true`)

## Impact

- Code: `packages/tiny-engineer-cli/src/commands/watch.js`, `src/daemon/reactions.js`, `src/robot/ops.js`, `src/robot/transport.js`, new shared module under `src/robot/`, related tests (`watch.test.js`, transport/ops tests if any).
- Scope for commits: `integrations`.
