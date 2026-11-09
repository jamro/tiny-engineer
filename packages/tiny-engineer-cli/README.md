# tiny-engineer

Unified CLI for the [Tiny Engineer](https://github.com/jamro/tiny-engineer) desk robot: install IDE hooks, map agent events to poses, drive animations over HTTP or USB serial, and run a host daemon for serial sessions and event rules.

## Install / run

```bash
npx -y tiny-engineer --help
npx -y tiny-engineer setup
npx -y tiny-engineer setup cursor --yes
npx -y tiny-engineer setup cursor --yes --transport serial
npx -y tiny-engineer doctor
npx -y tiny-engineer uninstall cursor --yes
npx -y tiny-engineer anim ring
npx -y tiny-engineer daemon status
npx -y tiny-engineer rules list
```

Supported IDEs: **Cursor**, **Claude Code**, **Antigravity**.

Cold `npx` can be slow: `setup` writes hook **`timeout` ≥ 30**. Prefer registry `npx -y tiny-engineer` over `npx github:…` (SSH often fails in IDE hooks).

## Commands (summary)

| Area | Commands |
| --- | --- |
| Hooks / setup | `setup`, `uninstall`, `doctor`, `hook <ide>` |
| Robot | `anim`, `play`, `settings`, `test`, `servo`, `park`, `health`, `reset` |
| Serial / events | `monitor`, `watch`, `port`, `daemon …`, `rules …` |

Full flag list: `tiny-engineer --help` and `tiny-engineer daemon --help`.

Serial commands auto-start a host **daemon** (local socket) unless disabled or the port is yielded. HTTP `anim` / hooks do not use the daemon. Event rules (`rules add|list|remove`) run a host program when the robot emits an event — see the integration guide.

## Config

Connection prefs (transport, URL, port, token) live in **user config** (`~/.config/tiny-engineer/env` on macOS/Linux, `%APPDATA%\tiny-engineer\env` on Windows; override with `TINY_ENGINEER_CONFIG_DIR`). `setup` never edits project `.env`.

Load order: flag → process env → project `.env` → user `env` → defaults. Host transport: `--transport http|serial` or `TINY_ENGINEER_TRANSPORT` (default `http`).

A user/process `TINY_ENGINEER_TOKEN` is **not** sent as `Authorization: Bearer` when the URL came only from project `.env` (request continues without Bearer; stderr warns). Serial skips Bearer. Details: [integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md).

## Docs

- [Integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md) (`setup` / hooks / daemon / rules)
- [USB serial](https://github.com/jamro/tiny-engineer/blob/main/docs/serial.md) (daemon, monitor, flash yield)
- [HTTP API](https://github.com/jamro/tiny-engineer/blob/main/docs/api.md) (custom agents / advanced)
- [CLI release process](https://github.com/jamro/tiny-engineer/blob/main/docs/cli-release.md) (maintainers: npm publish, separate from firmware `v*` tags)

## License

MIT — see [LICENSE](./LICENSE).
