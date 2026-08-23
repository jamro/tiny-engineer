import { getToken, loadDotEnv } from "./env.js";
import { animationForEvent } from "./map.js";
import { postAnim } from "./post.js";

const DEFAULT_URL = "http://tiny-engineer.local";

function printHelp() {
  console.log(`Usage: tiny-engineer-cursor [options]

Cursor hook helper: read event JSON from stdin, pick an animation, POST to the robot.

Options:
  --url <base>   Robot base URL (default: ${DEFAULT_URL})
  -h, --help     Show this help

Auth:
  If TINY_ENGINEER_TOKEN is set (process env or project-root .env), requests
  send Authorization: Bearer <token>. Must match the device access_token.
  No token → no Authorization header (device auth disabled).

Hook mode (no animation args):
  Cursor pipes JSON with hook_event_name (tool_name for preToolUse,
  status for stop). The CLI maps the event to reading / thinking / typing /
  ring / abort / error and POSTs \${url}/anim?name=…. Unknown or unmatched events
  exit 0 with no request.

Event map:
  sessionStart, beforeSubmitPrompt, beforeReadFile  → reading
  afterAgentThought, preCompact                     → thinking
  beforeShellExecution, subagentStart, afterFileEdit → typing
  stop + status aborted                             → abort
  stop + status error                               → error
  stop (completed / missing / other)                → ring
  preToolUse + Read                                 → reading
  preToolUse + edit/search/shell tools              → typing
  other preToolUse tools                            → (skip)

Examples:
  echo '{"hook_event_name":"stop","status":"completed"}' | tiny-engineer-cursor
  echo '{"hook_event_name":"stop","status":"aborted"}' | tiny-engineer-cursor
  echo '{"hook_event_name":"stop","status":"error"}' | tiny-engineer-cursor
  echo '{"hook_event_name":"preToolUse","tool_name":"Read"}' | tiny-engineer-cursor --url http://192.168.1.10
`);
}

/**
 * @param {string[]} argv
 * @returns {{ help: boolean, url: string, error?: string }}
 */
export function parseArgs(argv) {
  let url = DEFAULT_URL;
  let help = false;

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help: false, url, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help: false, url, error: "--url requires a value" };
      continue;
    }
    return { help: false, url, error: `Unknown argument: ${arg}` };
  }

  return { help, url };
}

function readStdin() {
  return new Promise((resolve) => {
    if (process.stdin.isTTY) {
      resolve("");
      return;
    }
    const chunks = [];
    process.stdin.setEncoding("utf8");
    process.stdin.on("data", (c) => chunks.push(c));
    process.stdin.on("end", () => resolve(chunks.join("")));
    process.stdin.on("error", () => resolve(""));
  });
}

/**
 * @param {string[]} argv
 */
export async function run(argv) {
  loadDotEnv();

  const opts = parseArgs(argv);
  if (opts.error) {
    console.error(opts.error);
    printHelp();
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    printHelp();
    return;
  }

  const raw = await readStdin();
  let event = null;
  if (raw.trim()) {
    try {
      event = JSON.parse(raw);
    } catch {
      process.exitCode = 0;
      return;
    }
  }

  const anim = animationForEvent(event);
  if (anim) {
    await postAnim(opts.url, anim, getToken());
  }
  process.exitCode = 0;
}
