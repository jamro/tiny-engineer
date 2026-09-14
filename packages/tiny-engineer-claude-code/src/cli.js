import { DEFAULT_URL, getBaseUrl, getToken, loadDotEnv } from "./env.js";
import { animationForEvent } from "./map.js";
import { postAnim } from "./post.js";

function printHelp() {
  console.log(`Usage: tiny-engineer-claude-code [options]

Claude Code hook helper: read event JSON from stdin, pick an animation, POST to the robot.

Options:
  --url <base>   Robot base URL (default: TINY_ENGINEER_URL, else ${DEFAULT_URL})
  -h, --help     Show this help

Auth:
  If TINY_ENGINEER_TOKEN is set (process env or project-root .env), requests
  send Authorization: Bearer <token>. Must match the device access_token.
  No token → no Authorization header (device auth disabled).
  A token from the process env is never sent to a TINY_ENGINEER_URL taken from
  the project .env; set the URL in the env or with --url instead.

Hook mode (no animation args):
  Claude Code pipes JSON with hook_event_name (plus tool_name for PreToolUse and
  source for SessionStart). The CLI maps the event to a pose and POSTs
  \${url}/anim?name=…. Unknown or unmatched events exit 0 with no request.
  Nothing is written to stdout, because Claude Code adds SessionStart /
  UserPromptSubmit stdout to the model's context.

Event map:
  SessionStart (source startup)                    → welcome
  SessionStart (any other or missing source)       → wakeup
  SessionEnd (reason clear/resume)                 → (skip)
  SessionEnd (any other reason)                    → sleep
  UserPromptSubmit                                 → reading
  PreToolUse + Read/Grep/Glob/WebFetch/WebSearch   → reading
  PreToolUse + Bash/PowerShell/Edit/Write/NotebookEdit → typing
  other PreToolUse tools                           → (skip)
  SubagentStart                                    → thinking
  PostToolBatch                                    → thinking
  PostToolUseFailure                               → error
  PreCompact                                       → thinking
  PermissionRequest                                → attention
  PermissionDenied                                 → abort
  Notification                                     → attention
  Stop                                             → ring
  StopFailure                                      → error

Examples:
  echo '{"hook_event_name":"Stop"}' | tiny-engineer-claude-code
  echo '{"hook_event_name":"PreToolUse","tool_name":"Read"}' | tiny-engineer-claude-code --url http://192.168.1.10
`);
}

/**
 * @param {string[]} argv
 * @returns {{ help: boolean, url?: string, error?: string }}
 */
export function parseArgs(argv) {
  let url;
  let help = false;

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help: false, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help: false, error: "--url requires a value" };
      continue;
    }
    return { help: false, error: `Unknown argument: ${arg}` };
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
    await postAnim(opts.url ?? getBaseUrl(), anim, getToken());
    // A timed-out fetch leaves its connect attempt open, which keeps Node alive
    // until undici's 10s connect timeout. Claude Code enforces no timeout on
    // async hooks, so exit now instead of lingering once per tool call.
    process.exit(0);
  }
  process.exitCode = 0;
}
