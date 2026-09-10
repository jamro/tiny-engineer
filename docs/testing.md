# Tests

Host checks. Neither flashes the board. GitHub Actions on `main` and PRs runs these same commands. On-device hardware: [hardware/testing.md](hardware/testing.md).

## Firmware

Unity on the host. Compiles `src/settings/validate.cpp` plus header-only helpers such as [`include/servos.h`](../include/servos.h).

```bash
pio test -e native
```

`pio run` builds firmware. Native tests are **`pio test -e native`**, not `pio run -e native`.

## Packages

Node 18+. No robot.

```bash
npm test --prefix packages/tiny-engineer-cursor
npm test --prefix packages/tiny-engineer-antigravity
```
