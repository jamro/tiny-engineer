# tiny-engineer

Unified CLI for the [Tiny Engineer](https://github.com/jamro/tiny-engineer) desk robot: install IDE hooks, map agent events to poses, and drive animations over **HTTP** or **USB serial**.

## Install / run

```bash
npx -y tiny-engineer setup
npx -y tiny-engineer setup cursor --yes
npx -y tiny-engineer doctor
npx -y tiny-engineer anim ring
npx -y tiny-engineer ports
npx -y tiny-engineer anim ring --serial /dev/cu.usbmodem14101
```

Supported IDEs: **Cursor**, **Claude Code**, **Antigravity**.

### Transport

| Mode | How |
| --- | --- |
| HTTP (default) | `--url` / `TINY_ENGINEER_URL` / `http://tiny-engineer.local` |
| Serial | `--serial <path>` or `TINY_ENGINEER_SERIAL` (wins over URL when set) |

Do not pass both `--url` and `--serial`. `play` requires HTTP (WAV body). Serial framing: [serial-api.md](https://github.com/jamro/tiny-engineer/blob/main/docs/serial-api.md).

## Docs

- [Integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md) (`setup` / hooks / transport)
- [HTTP API](https://github.com/jamro/tiny-engineer/blob/main/docs/api.md)
- [Serial API](https://github.com/jamro/tiny-engineer/blob/main/docs/serial-api.md)
- [CLI release process](https://github.com/jamro/tiny-engineer/blob/main/docs/cli-release.md) (maintainers: npm publish, separate from firmware `v*` tags)

## License

MIT — see [LICENSE](./LICENSE).
