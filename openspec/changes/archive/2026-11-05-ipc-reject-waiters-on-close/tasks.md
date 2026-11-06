# Tasks

## 1. Peer fail-fast on close

- [x] 1.1 In `packages/tiny-engineer-cli/src/daemon/ipc.js`, store settle handles (resolve/reject/timer) per request id; on socket `close` and `error`, reject all waiters with `daemon connection closed`, clear timers, and prevent double-settle; verify a mid-flight `request` with a long timeout rejects promptly when the peer socket is destroyed
- [x] 1.2 Keep alive-peer timeout behavior unchanged; verify a request still times out with the existing timeout error when the socket stays open and no response arrives

## 2. Suite check

- [x] 2.1 Run `npm test --prefix packages/tiny-engineer-cli` (or at least the new IPC test file plus `test/daemon.test.js`) and fix regressions; verify the suite exits 0
