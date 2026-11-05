# Design

## Context

See proposal.md — Why. `sourceHash(packageRoot)` walks `src/**/*.js` + `bin/tiny-engineer.js`, reads every file, SHA-256. `loadIdentity` always calls it. `decideDaemonAction` uses `sourceHash` only for same-root **dev** pairs; npm↔npm uses SemVer only (existing tests already keep same SemVer across different hashes).

Explore measured ~10–22 ms per hash on this checkout. One-shot CLI = one hash per process, so process memo alone is weak UX; **npm skip** removes the walk for published installs. Memo still helps tests and any multi-`loadIdentity` path.

## Goals / Non-Goals

**Goals:**

- B: npm `loadIdentity` never calls the content walk.
- A: memoize computed content hashes per `packageRoot` in-process; clearable for tests.
- Preserve existing `decideDaemonAction` outcomes and “edited checkout refreshes” for new processes.

**Non-Goals:**

- C: mtime/size invalidation inside a long-lived process.
- Changing SemVer/channel/proto replace rules.
- Disk-persisted hash cache across processes.

## Decisions

### 1. npm: sentinel hash, no walk

In `loadIdentity`, after `classifyChannel(packageRoot)`:

- If `npm` → `sourceHash: ""` (or a named constant e.g. `"npm"` — prefer `""` for simplicity) without calling `sourceHash()`.
- If `dev` → existing `sourceHash(packageRoot)` (memoized).

Daemon and client from the same generation both skip → matching sentinels. Mixed old daemon (real hex) + new npm client (`""`): npm↔npm path ignores hash → `keep`/`replace` still SemVer-only.

### 2. Memo inside `sourceHash`, clear for tests

Module-level `Map<string, string>` keyed by resolved `packageRoot`. On hit return cached digest; on miss compute, store, return.

Export `clearSourceHashCache()` (or equivalent) used by `daemon-policy` tests that mutate files mid-process. Production CLI one-shots need no clear.

**Why not memo only in `loadIdentity`:** `sourceHash` is also called directly from tests; one cache covers both.

**Why not mtime invalidation now:** out of scope (C); new process after edit is the supported refresh path.

### 3. Tests

- npm fixture / `node_modules/tiny-engineer` path: `loadIdentity` returns empty/sentinel hash; spy or timing optional — prefer assert sentinel + that `decideDaemonAction` unchanged.
- Memo: two `sourceHash` calls same root → equal; after clear + file edit → new hash (update existing “src bytes change” test to clear between mutate and rehash if memo would stale).
- Keep existing `decideDaemonAction` cases.

## Risks / Trade-offs

- [In-process edit without clear] → stale hash until process exit; acceptable for CLI; tests must clear.
- [Old npm daemon hex vs new client `""`] → ignored by decide for npm; status may look different — document in proposal impact only.
- [Constant collision with real empty] → hex digests are 64 chars; `""` cannot collide with a real digest.

## Migration Plan

1. Ship in CLI release; no on-disk migration.
2. Rollback: revert `policy.js` + tests.

## Open Questions

None.
