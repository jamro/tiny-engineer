# Cursor hooks

Project hooks drive Tiny Engineer poses from Cursor agent events. Files live under [`.cursor/`](../.cursor/).

## Setup

1. Flash firmware and join the board to Wi-Fi so `http://tiny-engineer.local` resolves.
2. Open this repo in Cursor (hooks run from the project root).
3. Enable **Hooks** in Cursor settings if they are off.
4. Confirm `.cursor/hooks/anim.sh` is executable:

```bash
chmod +x .cursor/hooks/anim.sh
```

5. Optional smoke test (robot should start typing):

```bash
.cursor/hooks/anim.sh typing
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
| `preToolUse` (edit/tool matchers) | `typing` | Agent is about to write, shell, search, task, etc. |
| `beforeReadFile` | `reading` | File is about to be read |
| `beforeShellExecution` | `typing` | Shell command is about to run |
| `subagentStart` | `typing` | Subagent starts |
| `afterFileEdit` | `typing` | Agent finished an edit |
| `stop` | `none` | Agent turn ends |

Script: [`.cursor/hooks/anim.sh`](../.cursor/hooks/anim.sh) — fire-and-forget `POST /anim?name=…` (2s timeout, does not block the agent). Config: [`.cursor/hooks.json`](../.cursor/hooks.json).

## Notes

- Robot offline → hook still exits 0; no agent stall.
- Animation API holds each pose ≥1s and keeps only the latest pending switch — see [`api.md`](api.md).
- The onboard RGB LED follows the active animation (white for typing/reading/thinking/welcome/ring, red for attention/error/abort, off for `none`) with 1 s fades — see [RGB LED](api.md#rgb-led).
- To change the map, edit `.cursor/hooks.json` (or pass another name to `anim.sh`).
