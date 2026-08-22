# Cursor hooks

Project hooks drive Tiny Engineer poses from Cursor agent events. Config lives in [`.cursor/hooks.json`](../.cursor/hooks.json). The animation CLI is [`packages/tiny-engineer-cursor/`](../packages/tiny-engineer-cursor/).

## Setup

1. Flash firmware and join the board to Wi-Fi so `http://tiny-engineer.local` resolves.
2. Open this repo in Cursor (hooks run from the project root).
3. Enable **Hooks** in Cursor settings if they are off.
4. Use **Node.js 18+** (hooks call `node packages/tiny-engineer-cursor/bin/tiny-engineer-cursor.js`).

5. Optional smoke test (robot should ring):

```bash
echo '{"hook_event_name":"stop"}' | node packages/tiny-engineer-cursor/bin/tiny-engineer-cursor.js
node packages/tiny-engineer-cursor/bin/tiny-engineer-cursor.js --help
```

Cursor reloads `.cursor/hooks.json` on save. If a hook never fires, restart Cursor and check the **Hooks** output channel.

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
| `stop` | `ring` | Agent turn ends |

Each anim hook runs the same command with **no animation args**. Cursor pipes event JSON on stdin; the CLI reads `hook_event_name` (and `tool_name` for `preToolUse`), maps to a pose in [`packages/tiny-engineer-cursor/src/map.js`](../packages/tiny-engineer-cursor/src/map.js), and `POST`s `/anim?name=…` (2s timeout, exit 0). Optional `--url` overrides the default `http://tiny-engineer.local`.

Config: [`.cursor/hooks.json`](../.cursor/hooks.json). Event logging (separate): [`.cursor/hooks/log-event.sh`](../.cursor/hooks/log-event.sh).

## Notes

- Robot offline → hook still exits 0; no agent stall.
- Animation API holds each pose ≥1s and keeps only the latest pending switch — see [`api.md`](api.md).
- The onboard RGB LED follows the active animation (white for typing/reading/thinking/welcome/ring, red for attention/error/abort, off for `none`) with 1 s fades — see [RGB LED](api.md#rgb-led).
- To change the map, edit [`packages/tiny-engineer-cursor/src/map.js`](../packages/tiny-engineer-cursor/src/map.js).
