import { readFileSync } from "node:fs";
import { basename } from "node:path";

import { DEFAULT_URL, getToken, loadDotEnv } from "./config/env.js";
import { get as getIntegration, list as listIntegrations } from "./integrations/registry.js";
import {
  formatRobotError,
  postAnim,
  postPlay,
  validateAnimName,
  validatePlayAnimName,
} from "./robot/client.js";
import { listSerialPorts, serialPlay } from "./robot/serial.js";
import { resolveTransport, transportLabel } from "./robot/transport.js";
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
  doctor                  Check URL/serial, token, hook wiring, and /health
  ports                   List serial port paths (for --serial / TINY_ENGINEER_SERIAL)
  hook <ide>              Read hook JSON from stdin, map to a pose, POST /anim
  anim <name> [--url|--serial]  POST /anim?name=<name>
  play <clip.wav> [opts]  POST /play (16-bit mono PCM WAV; HTTP or serial)

Options (most commands):
  --url <base>            Robot base URL (default: TINY_ENGINEER_URL, else ${DEFAULT_URL})
  --serial [path|auto]    USB CDC serial API (bare/--serial=auto = detect; caches under ~/.cache/tiny-engineer/)
  -q, --quiet, --silent   Suppress success/info lines; errors still print
  -h, --help              Show this help

Transport:
  Precedence: --serial → --url → TINY_ENGINEER_SERIAL → HTTP.
  TINY_ENGINEER_SERIAL=auto also triggers detection.
  Do not pass both --url and --serial.

Auth (HTTP only):
  If TINY_ENGINEER_TOKEN is set (process env or project-root .env), requests
  send Authorization: Bearer <token>. Must match the device access_token.
  Serial is a trusted local channel (no Bearer).

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
  tiny-engineer ports
  tiny-engineer anim ring
  tiny-engineer anim ring --serial
  tiny-engineer anim ring --serial /dev/cu.usbmodem14101
  tiny-engineer play clip.wav --serial=auto
  echo '{"hook_event_name":"stop"}' | tiny-engineer hook cursor
`);
}

/**
 * @param {import("./util/output.js").Output} out
 * @param {{ url?: string, serial?: string }} opts
 */
async function resolveOrFail(out, opts) {
  try {
    return await resolveTransport(opts);
  } catch (err) {
    out.error(err instanceof Error ? err.message : String(err));
    process.exitCode = 1;
    return null;
  }
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
    const transport = await resolveOrFail(localOut, {
      url: opts.url,
      serial: opts.serial,
    });
    if (transport) {
      // Fail-open: ignore result so an offline robot never stalls the agent.
      await postAnim(transport, anim, getToken());
    }
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

  const transport = await resolveOrFail(localOut, {
    url: opts.url,
    serial: opts.serial,
  });
  if (!transport) return;

  const result = await postAnim(transport, name, getToken());
  if (!result.ok) {
    localOut.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  localOut.ok(`animation ${name} → ${localOut.style.dim(transportLabel(transport))}`);
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

  const transport = await resolveOrFail(localOut, {
    url: opts.url,
    serial: opts.serial,
  });
  if (!transport) return;

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

  const pose = opts.name ?? "talking";
  /** @type {import("./robot/client.js").RobotResult} */
  let result;
  if (transport.type === "serial") {
    result = await serialPlay(transport.path, { wav, name: opts.name });
  } else {
    result = await postPlay({
      baseUrl: transport.baseUrl,
      wav,
      name: opts.name,
      token: getToken(),
    });
  }
  if (!result.ok) {
    localOut.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  localOut.ok(
    `played ${basename(/** @type {string} */ (opts.wavPath))} (${pose}) → ${localOut.style.dim(transportLabel(transport))}`,
  );
  process.exitCode = 0;
}

/**
 * @param {import("./util/output.js").Output} out
 * @param {string[]} argv
 */
async function runPorts(out, argv) {
  for (const arg of argv) {
    if (arg === "-h" || arg === "--help") {
      console.log(`Usage: tiny-engineer ports

List USB serial device paths for --serial / TINY_ENGINEER_SERIAL.
`);
      return;
    }
    if (arg.startsWith("-")) {
      out.error(`Unknown argument: ${arg}`);
      process.exitCode = 1;
      return;
    }
  }

  const ports = await listSerialPorts();
  if (ports.length === 0) {
    out.info("no serial ports found");
    process.exitCode = 0;
    return;
  }

  for (const p of ports) {
    const ids = [p.vendorId && `VID ${p.vendorId}`, p.productId && `PID ${p.productId}`]
      .filter(Boolean)
      .join(" ");
    const extra = [p.manufacturer, p.friendlyName, ids].filter(Boolean).join(" · ");
    out.ok(extra ? `${p.path}  ${out.style.dim(extra)}` : p.path);
  }
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
    case "ports":
      await runPorts(out, rest);
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
