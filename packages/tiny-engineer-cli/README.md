# tiny-engineer

Unified CLI for the [Tiny Engineer](https://github.com/jamro/tiny-engineer) desk robot: install IDE hooks, map agent events to poses, and drive animations over HTTP.

## Install / run

```bash
npx -y tiny-engineer setup
npx -y tiny-engineer setup cursor --yes
npx -y tiny-engineer doctor
npx -y tiny-engineer anim ring
```

Supported IDEs: **Cursor**, **Claude Code**, **Antigravity**.

## Docs

- [Integration guide](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md) (`setup` / hooks)
- [HTTP API](https://github.com/jamro/tiny-engineer/blob/main/docs/api.md) (custom agents / advanced)
- [CLI release process](https://github.com/jamro/tiny-engineer/blob/main/docs/cli-release.md) (maintainers: npm publish, separate from firmware `v*` tags)

## License

MIT — see [LICENSE](./LICENSE).
