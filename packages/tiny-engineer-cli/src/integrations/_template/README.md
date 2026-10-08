# Integration template

Copy this folder to `integrations/<id>/` and register the export in `registry.js`.

## Required files

| File | Role |
| --- | --- |
| `index.js` | Export an `Integration`: `id`, `name`, `parseHook`, `mapToAnim` |
| `map.js` | IDE event / tool → animation name |

Optional: `files.js` + `describeFiles` for stderr file logs; `respond` for stdout contracts; `hardExitAfterPost: true` if the IDE leaves Node hanging after `fetch` (Claude Code).

## Contract checklist

1. **`parseHook(argv, stdinText)`** — return `HookInput` or `null` to skip. Antigravity-style hosts that need a stdout reply should never return `null`.
2. **`mapToAnim(input)`** — return pose name or `null`.
3. **`respond(ctx)`** — Cursor/Claude: no-op (stdout must stay empty for Claude). Antigravity: print decision JSON always.
4. **`describeFiles(input, style)`** — stderr lines only; suppressed by `-q`.
5. Unit-test `map.js` under `test/map-<id>.test.js`.

## Wire-up

```js
// registry.js
import { myIde } from "./my-ide/index.js";
byId.set(myIde.id, myIde);
```

```bash
tiny-engineer hook <id>
# or, if the host passes the event on argv:
tiny-engineer hook <id> <EventName>
```
