# Tasks

## 1. Server and ensure

- [x] 1.1 Remove the `play-active` refusal on `daemon.stop` with `reason=replace` in `server.js` so shutdown runs during play; verify a unit/integration test can stop-for-replace while `link.playActive` and the process begins shutdown
- [x] 1.2 In `ensure.js`, delete the branch that returns the old connection on `play-active`; if that error still appears, fail closed (throw); verify `ensureDaemon` replace during an active play obtains a new hello instead of the old conn

## 2. Install alignment

- [x] 2.1 Remove `playActive` pre-check (and dead `play-active` handling after shutdown) from `installSupervisor`; simplify `commands.js` if it only probed status for that flag; verify `daemon install` during play succeeds and leaves one supervised daemon

## 3. Tests and docs

- [x] 3.1 Rewrite daemon/supervisor tests that expected replace or install to refuse during play (`replace waits while play is active`, install-during-play, and any “proto served during play” assumptions) so they assert abort + successful replace/install; verify `npm test --prefix packages/tiny-engineer-cli -- test/daemon.test.js test/supervisor.test.js` passes
- [x] 3.2 Grep docs for claims that replace/install wait on or preserve play; update `docs/serial.md` / `docs/integration.md` if needed; verify wording matches the new spec scenarios
- [x] 3.3 Run `npm test --prefix packages/tiny-engineer-cli` and fix regressions; verify the suite exits 0
