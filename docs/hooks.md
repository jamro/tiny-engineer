# Cursor hooks

Cursor is one IDE supported by the unified **`tiny-engineer` CLI**. Project hooks drive poses from Cursor agent events. Config: [`.cursor/hooks.json`](../.cursor/hooks.json). Install / overview: [`integration.md`](integration.md) (`npx -y tiny-engineer setup cursor`).

Published package: [`tiny-engineer` on npm](https://www.npmjs.com/package/tiny-engineer). Hook command: `npx -y tiny-engineer hook cursor`.

## Setup (this repo)

1. Flash firmware and join the board to Wi-Fi so `http://tiny-engineer.local` resolves.
2. Open this repo in Cursor (hooks run from the project root).
3. Enable **Hooks** in Cursor settings if they are off.
4. Use **Node.js 18+**.

Hooks in this repo call the **local** unified CLI via a portable wrapper (so CLI changes apply without waiting on npm, and without putting a machine-specific Node path in git):

```text
.cursor/hooks/tiny-engineer-cursor.sh
```

Cursor’s hook runner often has a thin `PATH` that omits nvm. Bare `node …` then fails with exit **127** / `command not found: node` while other hooks (e.g. [`log-event.sh`](../.cursor/hooks/log-event.sh)) still succeed. The wrapper sources nvm when needed, then falls back to common Homebrew/Volta locations, and `exec`s `packages/tiny-engineer-cli/bin/tiny-engineer.js hook cursor`.

Optional smoke test (robot should ring):

```bash
echo '{"hook_event_name":"stop"}' | .cursor/hooks/tiny-engineer-cursor.sh
echo '{"hook_event_name":"stop"}' | node packages/tiny-engineer-cli/bin/tiny-engineer.js hook cursor
node packages/tiny-engineer-cli/bin/tiny-engineer.js --help
```

Or run the wizard with an explicit local command:

```bash
node packages/tiny-engineer-cli/bin/tiny-engineer.js setup cursor --yes \
  --command 'node packages/tiny-engineer-cli/bin/tiny-engineer.js'
```

Cursor reloads `.cursor/hooks.json` on save. If a hook never fires or the robot stays still while event logging works, restart Cursor and check the **Hooks** output channel for exit 127 / `command not found: node`.

## Use in any Cursor project

Robot on Wi-Fi + Node 18+. Easiest:

```bash
npx -y tiny-engineer setup cursor --yes
```

Or hand-edit that project’s `.cursor/hooks.json`. Use this command for every anim hook:

```text
npx -y tiny-engineer hook cursor
```

Use `timeout` **≥ 30** (cold `npx` can exceed 2s).

```json
{
  "version": 1,
  "hooks": {
    "sessionStart": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "beforeSubmitPrompt": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "afterAgentThought": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "preCompact": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "preToolUse": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "beforeReadFile": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "beforeShellExecution": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "subagentStart": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "afterFileEdit": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ],
    "stop": [
      {
        "command": "npx -y tiny-engineer hook cursor",
        "timeout": 30
      }
    ]
  }
}
```

Same command every time — no animation args. Cursor pipes event JSON on stdin; the CLI picks the pose.

Smoke test / help:

```bash
npx -y tiny-engineer --help
echo '{"hook_event_name":"stop"}' | npx -y tiny-engineer hook cursor
```

Optional robot URL:

```bash
npx -y tiny-engineer hook cursor --url http://192.168.1.10
```

`-y` skips the install prompt. First run downloads from the npm registry; later runs use the npx cache.

### Why not `npx -y github:jamro/tiny-engineer`?

That shorthand makes npm fetch via **SSH** (`git@github.com:…`). Cursor hooks often have **no SSH agent** → exit 128, silent fail. Use the published package: `npx -y tiny-engineer`.

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

Each anim hook runs the same command with **no animation args**. Cursor pipes event JSON on stdin; the CLI reads `hook_event_name` (plus `tool_name` for `preToolUse`, `status` for `stop`), maps to a pose in [`packages/tiny-engineer-cli/src/integrations/cursor/map.js`](../packages/tiny-engineer-cli/src/integrations/cursor/map.js), and `POST`s `/anim?name=…` (2s HTTP timeout, exit 0). Optional `--url` overrides the default `http://tiny-engineer.local`. Events whose `conversation_id` or `generation_id` is `""` are skipped (no POST). Missing IDs still map.

If the robot has an `access_token` set, put the same value in `TINY_ENGINEER_TOKEN` (system/process env, or project-root `.env`). The CLI then sends `Authorization: Bearer <token>`. Without a token, no auth header is sent.

Config: [`.cursor/hooks.json`](../.cursor/hooks.json). Local anim hook: [`.cursor/hooks/tiny-engineer-cursor.sh`](../.cursor/hooks/tiny-engineer-cursor.sh). Event logging (separate): [`.cursor/hooks/log-event.sh`](../.cursor/hooks/log-event.sh).

## Notes

- Robot offline → hook still exits 0; no agent stall.
- Animation API holds each pose ≥1s and keeps only the latest pending switch — see [`api.md`](api.md).
- The onboard RGB LED follows the active animation (white for typing/reading/thinking/welcome/ring/wakeup, pulsing red for attention/error/dead, solid red for abort, off for `none`/`sleep`) with 1 s fades between non-pulse states — see [RGB LED](api.md#rgb-led).
- To change the map, edit [`packages/tiny-engineer-cli/src/integrations/cursor/map.js`](../packages/tiny-engineer-cli/src/integrations/cursor/map.js).
- Custom agents or non-Cursor IDEs: call the [REST API](integration.md#2-rest-api-advanced--custom).
