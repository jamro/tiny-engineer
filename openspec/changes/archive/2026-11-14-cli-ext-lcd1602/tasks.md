# Tasks

## 1. Serial settings lcd slice

- [x] 1.1 Extend serial `settings.get` to include `lcd1602`, `lcd1602_backlight_boot`, `lcd1602_boot`, `lcd1602_line1`, `lcd1602_line2` with the same types/defaults as HTTP; verify with a serial/HTTP get comparison or unit/fixture assertion on the JSON keys
- [x] 1.2 Extend serial `settings.set` to accept those keys (validation + `lcd1602ApplyFromSettings` when relevant); verify enable via serial then `settings.get` shows `lcd1602` true, and invalid `lcd1602_boot` → 400

## 2. Serial `ext` dispatcher

- [x] 2.1 Add shared invoke path so serial can run registered extension routes (Configured → enabled gate → handler) without duplicating LCD business logic; verify `pio run` compiles
- [x] 2.2 Implement serial `op: "ext"` (`id`, `suffix`, `method`, body) against the HTTP extension table; verify unknown id → 404, disabled LCD POST text → 503, PUT text → 405
- [x] 2.3 Verify LCD text get/set/clear and brightness get/set over serial match HTTP status/body shapes (including `backlight not found` when LED NACK)

## 3. CLI `ext` command

- [x] 3.1 Add `tiny-engineer ext <id> …` with transport resolution; v1 map only `lcd1602` (text get/set/clear, brightness get/set); verify unknown id fails locally without robot I/O
- [x] 3.2 Wire LCD resources through `viaTransport` (HTTP paths + serial `ext` fields); verify `npm test --prefix packages/tiny-engineer-cli` covers parsing/unknown id and a transport stub path
- [x] 3.3 Update CLI help/README examples for `ext lcd1602`; verify help text lists the new command

## 4. Docs and build

- [x] 4.1 Document serial `ext` and the `lcd1602*` settings slice in `docs/serial.md` (note remaining settings parity gap); verify ops list matches firmware
- [x] 4.2 Run `pio run` and CLI package tests; confirm both pass

## Workflow follow-up

- Archive after review/verify (`/opsx-verify` then `/opsx-archive`).
- Later change: full serial settings parity; register further extensions in the same `ext` table + CLI map.
