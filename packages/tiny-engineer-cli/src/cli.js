import { readFileSync } from "node:fs";
import { basename } from "node:path";

import { DEFAULT_URL, getBaseUrl, getToken, loadDotEnv } from "./config/env.js";
import { get as getIntegration, list as listIntegrations } from "./integrations/registry.js";
import {
  formatRobotError,
  postAnim,
  postPlay,
  validateAnimName,
  validatePlayAnimName,
} from "./robot/client.js";
import {
  parseAnimArgs,
  parseCommonArgs,
  parsePlayArgs,
  stripGlobalFlags,
} from "./util/args.js";
import { createOutput } from "./util/output.js";
import { createStyle, detectColor } from "./util/style.js";
import { readStdin } from "./util/stdin.js";
import { runDoctor } from "./wizard/doctor.js";
import { runSetup } from "./wizard/setup.js";
import { runUninstall } from "./wizard/uninstall.js";

function printHelp() {
  const integrations = listIntegrations().join(", ");
  console.log(`Usage: tiny-engineer <command> [options]

Unified CLI for the Tiny Engineer desk robot.

Commands:
  setup [ide...]          Merge IDE hook configs into the current project
  uninstall [ide...]      Remove Tiny Engineer hook entries (keeps other hooks)
  doctor                  Check URL, token, hook wiring, and /health
  hook <ide>              Read hook JSON from stdin, map to a pose, POST /anim
  anim <name> [--url]     POST /anim?name=<name>
  play <clip.wav> [opts]  POST /play (16-bit mono PCM WAV)

Options (most commands):
  --url <base>            Robot base URL (default: TINY_ENGINEER_URL, else ${DEFAULT_URL})
  -q, --quiet, --silent   Suppress success/info lines; errors still print
  -h, --help              Show this help

Auth:
  If TINY_ENGINEER_TOKEN is set (process env or project-root .env), requests
  send Authorization: Bearer <token>. Must match the device access_token.

Output:
  Colors/icons when stdout/stderr is a TTY (disabled by NO_COLOR or TERM=dumb).
  hook                    File touches on stderr; robot errors swallowed (exit 0).
                          Antigravity always prints decision JSON on stdout.
  anim / play             Friendly one-line success; exit 1 on errors.

Integrations (hook <ide>):
  ${integrations || "(none)"}

Examples:
  tiny-engineer setup cursor --yes
  tiny-engineer uninstall cursor --yes
  tiny-engineer uninstall --all --yes --dry-run
  tiny-engineer doctor
  tiny-engineer anim ring
  echo '{"hook_event_name":"stop"}' | tiny-engineer hook cursor
`);
}

/**
 * @param {import("./util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
async function runHook(out, argv, quietGlobal) {
  const opts = parseCommonArgs(argv);
  const quiet = quietGlobal || opts.quiet;
  const localOut = quiet === out.quiet ? out : createOutput({ quiet, style: out.style });

  if (opts.error) {
    localOut.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    printHelp();
    return;
  }

  const ide = opts.rest[0];
  const available = listIntegrations().join(", ") || "(none)";
  if (!ide) {
    localOut.error(`hook needs an integration id, e.g. tiny-engineer hook cursor (available: ${available})`);
    process.exitCode = 1;
    return;
  }

  const integration = getIntegration(ide);
  if (!integration) {
    localOut.error(`Unknown integration: ${ide} (available: ${available})`);
    process.exitCode = 1;
    return;
  }

  const stdinText = await readStdin();
  const input = integration.parseHook(opts.rest.slice(1), stdinText);
  if (!input) {
    process.exitCode = 0;
    return;
  }

  const anim = integration.mapToAnim(input);
  if (anim) {
    // Fail-open: ignore result so an offline robot never stalls the agent.
    await postAnim(opts.url ?? getBaseUrl(), anim, getToken());
  }

  const fileLines = integration.describeFiles?.(input, localOut.style) ?? [];
  for (const line of fileLines) {
    localOut.info(line);
  }

  integration.respond?.({ animPosted: Boolean(anim), anim, input });

  // Claude Code async hooks have no timeout; exit so undici connect attempts
  // cannot linger ~10s after a timed-out POST.
  if (integration.hardExitAfterPost && anim) {
    process.exit(0);
  }
  process.exitCode = 0;
}

/**
 * @param {import("./util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
async function runAnim(out, argv, quietGlobal) {
  const opts = parseAnimArgs(argv);
  const quiet = quietGlobal || opts.quiet;
  const localOut = quiet === out.quiet ? out : createOutput({ quiet, style: out.style });

  if (opts.error) {
    localOut.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    printHelp();
    return;
  }

  const name = /** @type {string} */ (opts.name);
  const invalid = validateAnimName(name);
  if (invalid) {
    localOut.error(invalid);
    process.exitCode = 1;
    return;
  }

  const baseUrl = opts.url ?? getBaseUrl();
  const result = await postAnim(baseUrl, name, getToken());
  if (!result.ok) {
    localOut.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  localOut.ok(`animation ${name} → ${localOut.style.dim(baseUrl)}`);
  process.exitCode = 0;
}

/**
 * @param {import("./util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
async function runPlay(out, argv, quietGlobal) {
  const opts = parsePlayArgs(argv);
  const quiet = quietGlobal || opts.quiet;
  const localOut = quiet === out.quiet ? out : createOutput({ quiet, style: out.style });

  if (opts.error) {
    localOut.error(opts.error);
    process.exitCode = 1;
    return;
  }

  if (opts.name) {
    const invalid = validatePlayAnimName(opts.name);
    if (invalid) {
      localOut.error(invalid);
      process.exitCode = 1;
      return;
    }
  }

  let wav;
  try {
    wav = readFileSync(opts.wavPath);
  } catch (err) {
    localOut.error(`cannot read ${opts.wavPath}: ${err.message}`);
    process.exitCode = 1;
    return;
  }

  const baseUrl = opts.url ?? getBaseUrl();
  const pose = opts.name ?? "talking";
  const result = await postPlay({
    baseUrl,
    wav,
    name: opts.name,
    token: getToken(),
  });
  if (!result.ok) {
    localOut.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  localOut.ok(
    `played ${basename(/** @type {string} */ (opts.wavPath))} (${pose}) → ${localOut.style.dim(baseUrl)}`,
  );
  process.exitCode = 0;
}

/**
 * @param {string[]} argv
 */
export async function run(argv) {
  loadDotEnv();

  const global = stripGlobalFlags(argv);
  const style = createStyle({
    color: detectColor(process.stdout) || detectColor(process.stderr),
  });
  const out = createOutput({ quiet: global.quiet, style });

  const args = global.rest;
  if (args.length === 0 || args[0] === "-h" || args[0] === "--help") {
    printHelp();
    return;
  }

  const [command, ...rest] = args;

  switch (command) {
    case "setup":
      await runSetup(out, rest);
      return;
    case "uninstall":
      await runUninstall(out, rest);
      return;
    case "doctor":
      await runDoctor(out, rest);
      return;
    case "hook":
      await runHook(out, rest, global.quiet);
      return;
    case "anim":
      await runAnim(out, rest, global.quiet);
      return;
    case "play":
      await runPlay(out, rest, global.quiet);
      return;
    default:
      out.error(`Unknown command: ${command}`);
      printHelp();
      process.exitCode = 1;
  }
}
