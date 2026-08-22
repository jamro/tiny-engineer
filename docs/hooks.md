# Cursor hooks

Project hooks drive Tiny Engineer poses from Cursor agent events. Config lives in [`.cursor/hooks.json`](../.cursor/hooks.json). CLI source: [`packages/tiny-engineer-cursor/`](../packages/tiny-engineer-cursor/). Root [`package.json`](../package.json) wraps that package for install via GitHub tarball.

For the big picture (REST vs Cursor CLI): [`integration.md`](integration.md).

## Setup (this repo)

1. Flash firmware and join the board to Wi-Fi so `http://tiny-engineer.local` resolves.
2. Open this repo in Cursor (hooks run from the project root).
3. Enable **Hooks** in Cursor settings if they are off.
4. Use **Node.js 18+**.

Hooks in this repo call the CLI via GitHub tarball `npx` (see [`.cursor/hooks.json`](../.cursor/hooks.json)). For local CLI development without waiting on GitHub, switch the command to:

```text
node packages/tiny-engineer-cursor/bin/tiny-engineer-cursor.js
```

Optional smoke test (robot should ring):

```bash
echo '{"hook_event_name":"stop"}' | node packages/tiny-engineer-cursor/bin/tiny-engineer-cursor.js
npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor --help
```

Cursor reloads `.cursor/hooks.json` on save. If a hook never fires, restart Cursor and check the **Hooks** output channel.

## Use in any Cursor project

Robot on Wi-Fi + Node 18+. In the other project’s `.cursor/hooks.json`, use this command for every anim hook (no clone of this repo required):

```text
npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor
```

Use `timeout` **≥ 30** (cold `npx` download can exceed 2s).

```json
{
  "version": 1,
  "hooks": {
    "sessionStart": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "beforeSubmitPrompt": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "afterAgentThought": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "preCompact": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "preToolUse": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "beforeReadFile": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "beforeShellExecution": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "subagentStart": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "afterFileEdit": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ],
    "stop": [
      {
        "command": "npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor",
        "timeout": 30
      }
    ]
  }
}
```

Same command every time — no animation args. Cursor pipes event JSON on stdin; the CLI picks the pose.

Smoke test / help:

```bash
npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor --help
echo '{"hook_event_name":"stop"}' | npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor
```

Optional robot URL:

```bash
npx -y --package=https://github.com/jamro/tiny-engineer/archive/refs/heads/main.tar.gz tiny-engineer-cursor --url http://192.168.1.10
```

`-y` skips the install prompt. First run downloads the tarball; later runs use the npx cache.

### Why not `npx -y github:jamro/tiny-engineer`?

That shorthand makes npm fetch via **SSH** (`git@github.com:…`). Cursor hooks often have **no SSH agent** → exit 128, silent fail. Also npm 10 needs the **bin name** after `--package=…` (`tiny-engineer-cursor`), and a **2s** hook timeout is too short for cold install.

HTTPS **tarball** URL avoids SSH and works in hooks.

## What ships in this repo

| Cursor event | Animation | When |
|---|---|---|
| `sessionStart` | `reading` | Cursor session starts |
| `beforeSubmitPrompt` | `reading` | You submit a prompt |
| `afterAgentThought` | `thinking` | Agent finishes a thinking block |
| `preCompact` | `thinking` | Context is about to compact |
| `preToolUse` (`Read`) | `reading` | Agent is about to read a file |
| `preToolUse` (edit/search/shell tools) | `typing` | Agent is about to write, shell, search, task, etc. |
| `beforeReadFile` | `reading` | File is about to be read |
| `beforeShellExecution` | `typing` | Shell command is about to run |
| `subagentStart` | `typing` | Subagent starts |
| `afterFileEdit` | `typing` | Agent finished an edit |
| `stop` (`status: completed` or missing) | `ring` | Agent turn completes |
| `stop` (`status: aborted`) | `abort` | Agent turn cancelled |
| `stop` (`status: error`) | `error` | Agent turn failed |

Each anim hook runs the same command with **no animation args**. Cursor pipes event JSON on stdin; the CLI reads `hook_event_name` (plus `tool_name` for `preToolUse`, `status` for `stop`), maps to a pose in [`packages/tiny-engineer-cursor/src/map.js`](../packages/tiny-engineer-cursor/src/map.js), and `POST`s `/anim?name=…` (2s HTTP timeout, exit 0). Optional `--url` overrides the default `http://tiny-engineer.local`.

Config: [`.cursor/hooks.json`](../.cursor/hooks.json). Event logging (separate): [`.cursor/hooks/log-event.sh`](../.cursor/hooks/log-event.sh).

## Notes

- Robot offline → hook still exits 0; no agent stall.
- Animation API holds each pose ≥1s and keeps only the latest pending switch — see [`api.md`](api.md).
- The onboard RGB LED follows the active animation (white for typing/reading/thinking/welcome/ring, red for attention/error/abort, off for `none`) with 1 s fades — see [RGB LED](api.md#rgb-led).
- To change the map, edit [`packages/tiny-engineer-cursor/src/map.js`](../packages/tiny-engineer-cursor/src/map.js).
- Root `package.json` is **Cursor CLI only**. Firmware stays PlatformIO; do not put build tooling in that package.
