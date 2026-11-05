# Proposal

## Why

Every `ensureDaemon` / daemon boot calls `loadIdentity` → `sourceHash`, which walks and reads all package `src/**/*.js` plus the bin entry. For **npm** installs, replace decisions already ignore that hash (SemVer / channel only), so the walk is pure cost. For **dev**, one-shot CLI processes gain little from memo alone, but repeated `loadIdentity` in the same process (tests, daemon boot paths) still re-hashes unchanged trees. Skip the walk on npm and memoize the hash per `packageRoot` in-process for everyone else.

## What Changes

- **npm channel:** `loadIdentity` MUST NOT walk/hash package sources; use a stable empty (or fixed) `sourceHash`. Replace/keep rules for npm stay SemVer/channel-based.
- **All channels:** memoize computed `sourceHash` per `packageRoot` within the process; expose a test-only clear so mutation tests stay correct.
- **Out of scope:** mtime/size invalidation (fingerprint C), changing replace semantics, IPC, or daemon protocol.

## Capabilities

### New Capabilities

- (none)

### Modified Capabilities

- `cli-host-daemon`: Clarify that npm clients/daemons do not need content hashing of package sources for identity, and that a process may reuse a previously computed source identity for the same package root (dev edit detection remains a new process or cleared cache after on-disk change in tests).

## Impact

- Code: `packages/tiny-engineer-cli/src/daemon/policy.js` (`sourceHash` / `loadIdentity`); `test/daemon-policy.test.js`.
- Hello/`daemon status` for npm may show a constant `sourceHash` instead of a content digest — not used for npm replace.
- Scope for commits: `integrations`.
