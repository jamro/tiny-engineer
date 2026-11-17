# Tasks

## 1. Settings and API surface

- [x] 1.1 Add `lcd1602_welcome_hello` through the settings checklist (default false, NVS `ext_lcd_hi`, patch/getter/HTTP/serial JSON, immediate apply); verify `pio test -e native` passes
- [x] 1.2 Update `docs/settings.md`, `docs/api.md`, `docs/serial.md`, and hardware testing settings lists for the new field; verify names match across docs
- [x] 1.3 Add Extensions UI toggle **Hello on welcome**, ApiReference row, types + `ui/test/robot.tsx`, and CLI parity key list; verify `npm run lint --prefix ui` and `npm test --prefix ui` pass

## 2. Overlay and queue

- [x] 2.1 Implement welcome-hello overlay state in LCD1602 (stash, show HELLO/HUMAN center without `persistLast`, restore or apply pending on end); verify flag false leaves text unchanged through a welcome
- [x] 2.2 Hook overlay start/end for every Welcome start and leave (natural finish and interrupt by another anim); verify boot welcome and `POST /anim?name=welcome` both show then restore
- [x] 2.3 Queue text/clear during overlay (last-wins, **200**, logical GET, persist only on apply after end); verify post-during-overlay then end shows queued text, and GET during overlay never returns HELLO/HUMAN
- [x] 2.4 End overlay immediately when extension or flag turns off mid-overlay; verify display returns to logical text
- [x] 2.5 Run `pio run` after firmware edits and verify the build succeeds

## Workflow follow-up

- Archive after implementation and review (`/opsx-archive`).
- Commit as `feat(settings): add LCD welcome hello overlay` (non-breaking).
